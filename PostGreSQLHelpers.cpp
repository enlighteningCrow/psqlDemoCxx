#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include <pqxx/pqxx>

#include <queue>
#include <thread>

#include <QApplication>
#include <QStandardItemModel>
#include <QTableView>
#include <QTimer>

#include "PostGreSQLHelpers.hpp"

#include "TGenerator.hpp"

using namespace pqxx;

void performQuery(work &tx, std::string_view query) { tx.exec(query); }

#ifdef USE_GUI

std::queue<result> results;

void displayQueries() {
  while (true) {
    if (!results.size()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
    int argc = 1;
    QApplication a(argc, nullptr);
    std::vector<QPair<QTableView *, QStandardItemModel *>> views;
    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&views]() {
      while (results.size()) {
        result &&r = std::move(results.front());
        // Get the column names
        std::vector<std::string> column_names(r.columns());

        auto &&rge = range(r.columns());

        std::transform(rge.begin(), rge.end(), column_names.begin(),
                       [&r](int i) { return r.column_name(i); });

        views.emplace_back(new QTableView(), new QStandardItemModel{});
        QTableView &view = *views.back().first;
        QStandardItemModel &model = *views.back().second;

        model.setColumnCount(r.columns());
        model.setRowCount(r.size());
        for (std::size_t i{0}; i < r.columns(); ++i) {
          model.setHeaderData(i, Qt::Horizontal,
                              QString::fromStdString(column_names[i]));
        }

        for (std::size_t i{0}; i < r.size(); ++i) {
          for (std::size_t j{0}; j < r.columns(); ++j) {
            model.setData(model.index(i, j),
                          r[i][j].is_null() ? ""
                                            : QString::fromStdString(
                                                  r[i][j].as<std::string>()));
          }
        }
        view.setModel(&model);
        view.show();
        results.pop();
      }
    });
    t.start(0);
    a.exec();
  }
}

void printQuery(work &tx, std::string_view query) {
  results.push(tx.exec(query));
}

#else

void printQuery(work &tx, std::string_view query) {
  pqxx::result &&r = tx.exec(query);
  // Get the column names
  std::vector<std::string> column_names(r.columns());

  auto &&rge = range(r.columns());

  std::transform(rge.begin(), rge.end(), column_names.begin(),
                 [&r](int i) { return r.column_name(i); });

  // Calculate the maximum width of each column
  std::vector<std::size_t> column_widths(column_names.size());
  std::transform(column_names.begin(), column_names.end(),
                 column_widths.begin(),
                 [](const std::string &s) { return s.size(); });
  for (auto const &row : r) {
    for (std::size_t i = 0; i < column_names.size(); ++i) {
      column_widths[i] =
          std::max(column_widths[i],
                   row[i].is_null() ? 0 : row[i].as<std::string>().length());
    }
  }

  std::vector<std::string> formatStrings;
  for (std::size_t i{0}; i < column_names.size(); ++i) {
    std::string type =
        tx.exec(fmt::format("select typname from pg_type where oid = {};",
                            pqxx::to_string(r.column_type(i))))[0][0]
            .as<std::string>();
    if (type == "varchar" || type == "char")
      formatStrings.push_back(fmt::format(" {{:<{}}} ", column_widths[i]));

    else
      formatStrings.push_back(fmt::format(" {{:>{}}} ", column_widths[i]));
  }

  std::string_view intersectionS = "+", normHorS = "|", normVerS = "-";

  auto gen = [&column_names, &column_widths]() -> Generator {
    for (std::size_t i{0}; i < column_names.size(); ++i) {
      co_yield fmt::format(
          fmt::runtime(fmt::format(" {{:^{}}} ", column_widths[i])),
          column_names[i]);
    }
  };

  fmt::print("{}\n", fmt::join(gen(), normHorS));

  auto &&sepLines = [&column_widths]() -> Generator {
    for (std::size_t i : column_widths) {
      co_yield std::string(i + 2, '-');
    }
  };

  fmt::print("{}\n", fmt::join(sepLines(), intersectionS));

  for (const auto &row : r) {
    for (std::size_t i{0}; i < formatStrings.size(); ++i) {
      if (i)
        fmt::print("{}", normHorS);
      fmt::print(fmt::runtime(formatStrings[i]),
                 row[i].is_null() ? "" : row[i].as<std::string>());
    }
    fmt::print("\n");
  }

  fmt::print("\n\n");
}

#endif

void printRelations(work &tx) {
  // print out all non-system relations
  printQuery(tx, "SELECT relname FROM pg_catalog.pg_class WHERE relkind='r' "
                 "AND relname !~ '^(pg_|sql_)';");
}

void printTableColumnNames(work &tx, std::string_view relationName) {
  // print out the column names
  result &&r =
      tx.exec(fmt::format("select * from \"{}\" limit 0;", relationName));
  for (std::size_t i{0}; i < r.columns(); ++i)
    fmt::print("{}\n", r.column_name(i));
}

void printTableContents(work &tx, std::string_view relationName) {
  // print out the contents of the table
  printQuery(tx, fmt::format("select * from \"{}\";", relationName));
}
