#include <iostream>

#include <pqxx/pqxx>

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <string>
#include <thread>

#include "PostGreSQLHelpers.hpp"
#include "TGenerator.hpp"
#include "macroHelpers.hpp"

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
#ifdef USE_GUI
  std::thread t{displayQueries};
  t.detach();
#endif
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
