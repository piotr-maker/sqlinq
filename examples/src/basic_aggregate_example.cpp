/**
 * Aggregate Functions Example
 *
 * Demonstrates usage of aggregate functions like COUNT, SUM, AVG, MIN, MAX.
 * Useful for analytics queries without fetching all rows.
 */

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <sqlinq/config.hpp>
#include <sqlinq/database.hpp>
#include <sqlinq/query.hpp>
#include <sqlite_backend.hpp>

#include "model.hpp"

using namespace std;

int main() {
  auto db_path = std::filesystem::path{SQLITE_DB_FILE};
  sqlinq::DatabaseConfig cfg{};
  cfg.database = db_path.string();

  sqlinq::SQLiteBackend sqlite;
  sqlite.connect(cfg);

  sqlinq::Database db{sqlite};

  // count all records
  {
    int64_t employee_count{};
    auto q = sqlinq::Query<Employees>().select(sqlinq::count());
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
    auto q = sqlinq::Query<Employees>().select(
        sqlinq::count(&Employees::manager).distinct());
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
