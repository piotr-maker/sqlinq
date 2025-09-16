/**
 * Aggregate Functions Example
 *
 * Demonstrates usage of aggregate functions like COUNT, SUM, AVG, MIN, MAX.
 * Useful for analytics queries without fetching all rows.
 */

#include <cstdlib>
#include <exception>
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

  // count all records
  {
    int64_t employee_count{};
    auto q = Query<Employees>().select(count());
    try {
      auto cursor = db.execute(q);
      cursor.next();
      employee_count = std::get<0>(cursor.current());
    } catch (const std::exception &ex) {
      std::cout << "Count query failed: " << ex.what() << '\n';
    }
    std::cout << "Employees Count(*): " << employee_count << '\n';
  }

  // count records by column
  {
    int64_t manager_count{};
    auto q = Query<Employees>().select(count(&Employees::manager).distinct());
    try {
      auto cursor = db.execute(q);
      cursor.next();
      manager_count = std::get<0>(cursor.current());
    } catch (const std::exception &ex) {
      std::cout << "Count by column query failed: " << ex.what() << '\n';
    }
    std::cout << "Employees Count(manager_id): " << manager_count << '\n';
  }
  return EXIT_SUCCESS;
}
