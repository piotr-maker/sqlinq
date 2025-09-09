/**
 * ORM Example
 *
 * Demonstrates how to use library in a simple ORM-like style. Covers basic CRUD
 * operations: create, find, get_all, update, remove.
 */

#include <cstdlib>
/*#include <sqlinq/config.hpp>*/
#include <sqlinq/database.hpp>
#include <sqlinq/query.hpp>

#include "model.hpp"
#include "mysql_backend.hpp"
#include "sqlite_backend.hpp"

using namespace std;
using namespace sqlinq;

int main() {
  MySQLBackend mysql;
  mysql.connect("localhost", "piotr", "passwd", "personnel");
  SQLiteBackend sqlite;
  sqlite.connect("personnel.sqlite3");

  Database db{mysql};

  Jobs job{.id = 0,
           .title = "Software Engineer",
           .min_salary = sqlinq::Decimal<8, 2>{"2000.00"},
           .max_salary = sqlinq::Decimal<8, 2>{"7000.00"}};
  db.create(job);
  std::cout << "New job created with id: " << job.id << '\n';

  // Find by primary key
  std::optional<Jobs> result = db.find<Jobs>(job.id);
  if (result.has_value()) {
    std::cout << "Found job: " << result->title << '\n';
  }

  // Get all records
  auto jobs = db.get_all<Jobs>();
  for (auto &j : jobs) {
    std::cout << j.id << ' ' << j.title << '\n';
  }

  // Update record
  if (result) {
    result->max_salary = Decimal<8, 2>{"9000.00"};
    db.update(*result);
  }

  // Delete record
  if (result) {
    db.remove<Jobs>(result->id);
  }
  return EXIT_SUCCESS;
}
