/**
 * LINQ-style Query Example
 *
 * Demonstrates query building in a LINQ-like fluent API style. Shows how to
 * select, filter and sort records without writing raw SQL.
 */

#include <cstdlib>
#include <exception>
#include <mysql_backend.hpp>
#include <sqlinq/config.hpp>
#include <sqlinq/database.hpp>
#include <sqlinq/query.hpp>

#include "model.hpp"
#include "record.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: " << '\n';
    std::cout << "  " << argv[0] << " /path/to/config" << '\n';
    return EXIT_FAILURE;
  }

  auto cfgs = sqlinq::parse_config_file(argv[1]);
  sqlinq::MySQLBackend mysql;
  try {
    mysql.connect(cfgs.at("mysql"));
  } catch(const std::exception &ex) {
    std::cout << ex.what() << '\n';
    return EXIT_FAILURE;
  }

  sqlinq::Database db{mysql};

  // simple select model with filtering and sorting
  {
    auto q = sqlinq::Query<Jobs>()
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
      auto tup = sqlinq::structure_to_tuple(job);
      print_record(std::forward<decltype(tup)>(tup));
    }
    std::cout << '\n';
  }

  // select data from specific columns
  {
    auto q = sqlinq::Query<Employees>()
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
    auto q = sqlinq::Query<Employees>()
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
    auto q = sqlinq::Query<Countries>().insert(
        [](auto &c) { c.id = "PL", c.name = "Poland", c.region = 1; });
    try {
      db.execute(q);
    } catch (const std::exception &ex) {
      std::cout << "Failed to create country: " << ex.what() << '\n';
    }
  }

  // delete country
  {
    auto q = sqlinq::Query<Countries>().remove().where(
        [](const auto &c) { return c.id == "PL"; });
    try {
      db.execute(q);
    } catch (const std::exception &ex) {
      std::cout << "Failed to delete country: " << ex.what() << '\n';
    }
  }
  return EXIT_SUCCESS;
}
