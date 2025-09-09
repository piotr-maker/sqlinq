/**
 * LINQ-style Query Example
 * 
 * Demonstrates query building in a LINQ-like fluent API style. Shows how to
 * select, filter and sort records without writing raw SQL.
 */

#include <cstdlib>
#include <exception>
/*#include <sqlinq/config.hpp>*/
#include <sqlinq/database.hpp>
#include <sqlinq/query.hpp>

#include "model.hpp"
#include "mysql_backend.hpp"
#include "record.hpp"
#include "sqlite_backend.hpp"

using namespace std;
using namespace sqlinq;

int main() {
  MySQLBackend mysql;
  mysql.connect("localhost", "piotr", "passwd", "personnel");
  SQLiteBackend sqlite;
  sqlite.connect("personnel.sqlite3");

  Database db{mysql};

  // simple select model with filtering and sorting
  {
    auto q = Query<Jobs>()
                 .select_all()
                 .order_by(&Jobs::max_salary)
                 .where([](const auto &job) { return job.id > 8; });
    std::vector<Jobs> jobs;
    try {
      jobs = db.to_vector(q);
    } catch (const exception &ex) {
      std::cout << "Failed to select model entity: " << ex.what() << '\n';
    }

    for (const auto &job : jobs) {
      auto tup = structure_to_tuple(job);
      print_record(std::forward<decltype(tup)>(tup));
    }
    std::cout << '\n';
  }

  // select data from specific columns
  {
    auto q = Query<Employees>()
                 .select(&Employees::id, &Employees::first_name,
                         &Employees::last_name)
                 .order_by(&Employees::last_name)
                 .where([](const auto &e) {
                   return e.phone_number == std::nullopt;
                 });
    try {
      for (auto &row : db.execute(q)) {
        print_record(std::forward<std::decay_t<decltype(row)>>(row));
      }
    } catch (const std::exception &ex) {
      std::cout << "Select from specific columns failed: " << ex.what() << '\n';
    }

    std::cout << '\n';
  }

  // update employee
  {
    auto q = Query<Employees>()
                 .update([](auto &e) { e.phone_number = "515.123.4568"; })
                 .where([](const auto &e) { return e.id == 100; });
    try {
      db.execute(q);
    } catch (const std::exception &ex) {
      std::cout << "Failed to update phone_number: " << ex.what() << '\n';
    }
  }

  // create country
  {
    auto q = Query<Countries>().insert(
        [](auto &c) { c.id = "PL", c.name = "Poland", c.region = 1; });
    try {
      db.execute(q);
    } catch (const std::exception &ex) {
      std::cout << "Failed to create country: " << ex.what() << '\n';
    }
  }

  // delete country
  {
    auto q = Query<Countries>().remove().where(
        [](const auto &c) { return c.id == "PL"; });
    try {
      db.execute(q);
    } catch (const std::exception &ex) {
      std::cout << "Failed to delete country: " << ex.what() << '\n';
    }
  }
  return EXIT_SUCCESS;
}
