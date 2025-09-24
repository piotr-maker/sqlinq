/**
 * ORM Example
 *
 * Demonstrates how to use library in a simple ORM-like style. Covers basic CRUD
 * operations: create, find, get_all, update, remove.
 */

#include <cstdlib>
#include <filesystem>
#include <sqlinq/database.hpp>
#include <sqlinq/query.hpp>
#include <sqlinq/sqlite_backend.hpp>

#include "model.hpp"

using namespace std;

int main() {
  auto db_path = std::filesystem::path{SQLITE_DB_FILE};
  sqlinq::DatabaseConfig cfg{};
  cfg.database = db_path.string();

  sqlinq::SQLiteBackend sqlite;
  sqlite.connect(cfg);

  sqlinq::Database db{sqlite};

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
  for (auto &j : db.get_all<Jobs>()) {
    std::cout << j.id << ' ' << j.title << '\n';
  }

  // Update record
  if (result) {
    result->max_salary = sqlinq::Decimal<8, 2>{"9000.00"};
    db.update(*result);
  }

  // Delete record
  if (result) {
    db.remove<Jobs>(result->id);
  }
  return EXIT_SUCCESS;
}
