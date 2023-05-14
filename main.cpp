#include <cmath>
#include <coroutine>
#include <iostream>
#include <ranges>
#include <sstream>

#include <pqxx/connection.hxx>
#include <pqxx/pqxx>

#include <fmt/core.h>
#include <fmt/os.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <string>
#include <string_view>

#include "macroHelpers.hpp"

using namespace pqxx;
using u16 = uint16_t;
using u8 = uint8_t;

#define BEGIN
#define END
#define prnt(...) __VA_OPT__(fmt::print(__VA_ARGS__))

template <class T> class TGenerator {
public:
  struct promise_type {
    T val;
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    std::suspend_always yield_value(T value) {
      val = std::move(value);
      return {};
    }
    auto get_return_object() {
      return TGenerator(
          std::coroutine_handle<promise_type>::from_promise(*this));
    }
    void unhandled_exception() {}
  };

private:
  std::coroutine_handle<promise_type> handle_;

public:
  TGenerator(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

  ~TGenerator() {
    if (handle_) {
      handle_.destroy();
    }
  }

  class iterator {
    std::coroutine_handle<promise_type> handle_;

  public:
    typedef std::forward_iterator_tag iterator_category;
    typedef iterator self_type;
    typedef T value_type;

    iterator() = default;

    iterator(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

    iterator &operator++() {
      handle_.resume();
      if (handle_.done()) {
        handle_ = nullptr;
      }
      return *this;
    }

    bool operator==(const iterator &other) const {
      return handle_ == other.handle_;
    }

    bool operator!=(const iterator &other) const { return !(*this == other); }

    const T &operator*() const { return handle_.promise().val; }
  };

  iterator begin() {
    if (handle_) {
      handle_.resume();
    }
    return iterator(handle_);
  }

  iterator end() { return iterator(); }
};

template <class T>
  requires std::integral<T>
TGenerator<T> range(T start, T end, T step) {
  for (T i{start}; step > 0 ? i < end : i > end; i += step) {
    co_yield i;
  }
}

template <class T>
  requires std::integral<T>
TGenerator<T> range(T start, T end) {
  for (T i{start}; i < end; ++i) {
    co_yield i;
  }
}

template <class T>
  requires std::integral<T>
TGenerator<T> range(T end) {
  for (T i{0}; i < end; ++i) {
    co_yield i;
  }
}

using Generator = TGenerator<std::string>;

void performQuery(work &tx, std::string_view query) { tx.exec(query); }

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

void printRelations(work &tx) {
  BEGIN
  // print out all non-system relations
  printQuery(tx, "SELECT relname FROM pg_catalog.pg_class WHERE relkind='r' "
                 "AND relname !~ '^(pg_|sql_)';");
  END
}

void printTableColumnNames(work &tx, std::string_view relationName) {
  BEGIN
  // print out the column names
  result &&r =
      tx.exec(fmt::format("select * from \"{}\" limit 0;", relationName));
  for (std::size_t i{0}; i < r.columns(); ++i)
    fmt::print("{}\n", r.column_name(i));

  END
}

void printTableContents(work &tx, std::string_view relationName) {
  // print out the contents of the table
  printQuery(tx, fmt::format("select * from \"{}\";", relationName));
}

template <class T> void readInto(T &t, std::string_view pDesc) {
  while (!(std::cin >> t)) {
    if (std::cin.eof())
      std::exit(0);
    std::cout << "Error: Invalid input. Please try again.\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Enter the " << pDesc << ":\t";
  }
}
template <> void readInto<std::string>(std::string &t, std::string_view pDesc) {
  while (!std::getline(std::cin >> std::ws, t)) {
    if (std::cin.eof())
      std::exit(0);
    std::cout << "Error: Invalid input. Please try again.\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Enter the " << pDesc << ":\t";
  }
}

#define COMMA ,

std::vector<std::pair<std::string, std::function<void(work &)>>> menu{
    // print out all non-system relations
    {"List available relations", [](work &tx) { printRelations(tx); }},

    // print out the "PRESIDENT' relation
    {"List table contents",
     makeLambda(printTableContents(tx, tableName);
                , std::string, tableName, "name of the table")},

    // Get total number of votes each presidential candidate has received in
    // all elections
    {"List total number of votes each presidential candidate has received "
     "in all elections",
     [](work &tx) {
       printQuery(
           tx,
           R"(select "CANDIDATE" "Candidate name", sum("VOTES") "Sum of votes" from "ELECTION" group by "CANDIDATE" order by "Sum of votes" asc;)");
     }},

    // Get number of presidents born from each state in descending order

    {"Get number of presidents born from each state in descending order",
     [](work &tx) {
       printQuery(
           tx,
           R"(select "PRES_NAME", "DEATH_AGE" from "PRESIDENT" p where (select count(*) from "PRESIDENT" q where q."DEATH_AGE" < p."DEATH_AGE") = 2;)");
     }},

    // Write the details of the youngest president to a file
    {"Write the details of a president to a file",
     makeLambda(
         pqxx::result &&r = tx.exec(fmt::format(
             R"(select * from "PRESIDENT" p where p."PRES_NAME" = '{}';)",
             presName));
         if (r.empty()) {
           throw std::runtime_error("No president with name " +
                                    std::string(presName) + " found.");
           return;
         } std::ofstream file("president_" + presName + ".txt");
         if (!r[0][0].is_null()) file
         << fmt::format("Name: {}\n" COMMA r[0][0].as<std::string>());
         if (!r[0][1].is_null()) file
         << fmt::format("Birth Year: {}\n" COMMA r[0][1].as<int>());
         if (!r[0][2].is_null()) file
         << fmt::format("Years Served: {}\n" COMMA r[0][2].as<int>());
         if (!r[0][3].is_null()) file
         << fmt::format("Death Age: {}\n" COMMA r[0][3].as<int>());
         if (!r[0][4].is_null()) file
         << fmt::format("Party: {}\n" COMMA r[0][4].as<std::string>());
         if (!r[0][5].is_null()) file
         << fmt::format("State Born: {}\n" COMMA r[0][5].as<std::string>());
         , std::string, presName, "name of the president")},

    // Write the details of a president to a json file
    {"Write the details of a president to a json file",
     makeLambda(
         pqxx::result &&r = tx.exec(fmt::format(
             R"(select * from "PRESIDENT" p where p."PRES_NAME" = '{}';)",
             presName));
         if (r.empty()) {
           throw std::runtime_error("No president with name " +
                                    std::string(presName) + " found.");
           return;
         } std::ofstream file("president_" + presName + ".json");
         file << "{\n";
         if (!r[0][0].is_null()) file << fmt::format(
             "\t\"Name\": \"{}\",\n" COMMA r[0][0].as<std::string>());
         if (!r[0][1].is_null()) file
         << fmt::format("\t\"Birth Year\": {},\n" COMMA r[0][1].as<int>());
         if (!r[0][2].is_null()) file
         << fmt::format("\t\"Years Served\": {},\n" COMMA r[0][2].as<int>());
         if (!r[0][3].is_null()) file
         << fmt::format("\t\"Death Age\": {},\n" COMMA r[0][3].as<int>());
         if (!r[0][4].is_null()) file << fmt::format(
             "\t\"Party\": \"{}\",\n" COMMA r[0][4].as<std::string>());
         if (!r[0][5].is_null()) file << fmt::format(
             "\t\"State Born\": \"{}\"\n" COMMA r[0][5].as<std::string>());
         file << "}";, std::string, presName, "name of the president")},

    // For all the presidents who share at least 1 hobby with at least 2
    // other presidents and had a spouse who is not more than 2 years
    // younger that them, list their names, the name of their spouse, and
    // the hobby they share
    {"For all the presidents who share at least 1 hobby with at least 2 "
     "other "
     "presidents and had a spouse who is not more than 2 years younger "
     "than "
     "them, list their names, the name of their spouse, and the hobby they "
     "share",
     [](work &tx) {
       printQuery(tx, R"(
select p."PRES_NAME" "President Name", 
max(m."SPOUSE_NAME") "Spouse Name",
max(h."HOBBY") "HOBBY"
from "PRESIDENT" p
join "PRES_MARRIAGE" m on p."PRES_NAME" = m."PRES_NAME"
join "PRES_HOBBY" h on p."PRES_NAME" = h."PRES_NAME"
where (select count(distinct ph."PRES_NAME") 
  from "PRES_HOBBY" ph
  where ph."HOBBY" = h."HOBBY"
  and ph."PRES_NAME" != h."PRES_NAME") >= 2
and m."PR_AGE" - m."SP_AGE" <= 2
group by p."PRES_NAME"
;
)");
     }},

    // Insert a new president
    {"Insert a new president",
     makeLambda(performQuery(tx COMMA fmt::format(
         R"(insert into "PRESIDENT" ("PRES_NAME", "BIRTH_YR", "YRS_SERV", "DEATH_AGE", "PARTY", "STATE_BORN") VALUES ('{}', {}, {}, '{}', '{}');)",
         presName, birthYr, yrsServ, deathAge, party, stateBorn));
                , std::string, presName, "name of the president", int, birthYr,
                "birth year of the president", int, yrsServ,
                "years served by the president", int, deathAge,
                "death age of the president", std::string, party,
                "party of the president", std::string, stateBorn,
                "state the president was born in")},

    // Update the president's name
    {"Update the president's name",
     makeLambda(performQuery(tx COMMA fmt::format(
         R"(update "PRESIDENT" set "PRES_NAME" = '{}' where "PRES_NAME" = '{}';)",
         presName, oldPresName));
                , std::string, presName, "new name of the president",
                std::string, oldPresName, "old name of the president")},

    // Delete a president
    {"Delete a president",
     makeLambda(performQuery(tx COMMA fmt::format(
         R"(delete from "PRESIDENT" where "PRES_NAME" = '{}';)", presName));
                , std::string, presName, "name of the president")},
    //
};

int main(int argc, char **argv) {

  try {
    connection conn{""};

    while (true) {
      work tx{conn};
      try {
        fmt::print(
            "Enter a number to select an option (Enter -1 to view menu): ");
        int choice;
        while (!(std::cin >> choice)) {
          if (std::cin.eof())
            return 0;
          std::cout << "Error: Invalid input. Please try again.\n";
          std::cin.clear();
          std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          std::cout << "Enter a number to select an option: ";
        }
        if (choice == -1) {
          fmt::print("Options:\n");
          for (std::size_t i{0}; i < menu.size(); ++i) {
            fmt::print("{}: {}\n", i, menu[i].first);
          }
          fmt::print("\n\n");
          continue;
        }
        if (choice < -1 || choice >= menu.size()) {
          fmt::print("Invalid choice. Please try again.\n");
          continue;
        }
        menu[choice].second(tx);
        tx.commit();
      } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        tx.abort();
      }
    }

  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}
