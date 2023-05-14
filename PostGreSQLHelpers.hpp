#include <pqxx/pqxx>
#include <queue>

#define USE_GUI

using namespace pqxx;

void displayQueries();

void performQuery(work &tx, std::string_view query);

void printQuery(work &tx, std::string_view query);

void printRelations(work &tx);

void printTableColumnNames(work &tx, std::string_view relationName);

void printTableContents(work &tx, std::string_view relationName);

extern std::queue<result> results;
