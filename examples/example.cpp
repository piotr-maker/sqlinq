#include <cstdlib>
#include <exception>
#include <sqlinq/config.hpp>
#include <sqlinq/query.hpp>
#include <string>

#include "model.hpp"
#include "record.hpp"

using namespace std;
using namespace sqlinq;

int main() {
#if SQLINQ_PLUGIN == SQLINQ_PLUGIN_MYSQL
  mysql::Database db;
  db.connect("localhost", "piotr", "passwd", "personnel");
#elif SQLINQ_PLUGIN == SQLINQ_PLUGIN_SQLITE
  sqlite::Database db;
  db.connect("database.db");
#endif

  Query<plugin::Database, Jobs> jobs_query{db};
  Query<plugin::Database, Employees> employees_query{db};

  // select record from model entity
  {
    std::vector<Jobs> jobs;
    try {
      jobs = jobs_query.select();
    } catch (const exception &ex) {
      std::cout << "Failed to select model entity: " << ex.what() << '\n';
    }

    for (const auto job : jobs) {
      auto tup = structure_to_tuple(job);
      print_record(std::forward<decltype(tup)>(tup));
    }
    std::cout << '\n';
  }

  // select data from specific columns with order_by
  {
    vector<tuple<int, optional<string>, string, decimal<8, 2>>> employees;
    try {
      employees = employees_query.order_by("first_name")
                      .select<int, optional<string>, string, decimal<8, 2>>(
                          {"employee_id", "first_name", "last_name", "salary"});

    } catch (const std::exception &ex) {
      std::cout << "Select from specific columns failed: " << ex.what() << '\n';
    }

    for (auto e : employees) {
      print_record(std::forward<decltype(e)>(e));
    }
    std::cout << '\n';
  }

  // count all records
  {
    std::size_t count;
    try {
      count = employees_query.count();
    } catch (const std::exception ex) {
      std::cout << "Count query failed: " << ex.what() << '\n';
    }
    std::cout << "Employees Count(*): " << count << '\n';
  }

  // count records by column
  {
    std::size_t count;
    try {
      count = employees_query.count("phone_number");
    } catch (const std::exception ex) {
      std::cout << "Count by column query failed: " << ex.what() << '\n';
    }
    std::cout << "Employees Count(phone_number): " << count << '\n';
  }

  // advanced count
  {
    vector<tuple<size_t, size_t>> result;
    try {
      result = employees_query.group_by("department_id")
                   .select<size_t, size_t>({"department_id", "COUNT(*)"});
    } catch (const std::exception ex) {
      std::cout << "Advanced count query failed: " << ex.what() << '\n';
    }

    for (auto record : result) {
      print_record(std::forward<decltype(record)>(record));
    }
    std::cout << '\n';
  }
  return EXIT_SUCCESS;
}
