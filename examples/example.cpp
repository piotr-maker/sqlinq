#include <cstdlib>
#include <sqlinq/query.h>
#include <sqlinq/mysql/database.h>
#include <sqlinq/sqlite/database.h>

#include "model.hpp"
#include "record.hpp"

using namespace sqlinq;

int main() {
  sqlite::database db;
  db.connect("database.db");

  query<sqlite::database, Jobs> jobs_query{db};
  {
    auto jobs = jobs_query.select();
    if(jobs.has_value()) {
      for(const auto job : jobs.value()) {
        auto tup = structure_to_tuple(job);
        print_record(std::forward<decltype(tup)>(tup));
      }
      std::cout << '\n';
    } else {
      std::cout << "Error: " << jobs.error().message() << '\n';
    }
  }

  // select with order_by
  query<sqlite::database, Employees> employees_query{db};
  {
    auto employees = employees_query
      .order_by("first_name")
      .select<int, std::optional<std::string>, std::string, decimal<8,2>>(
        {"employee_id", "first_name", "last_name", "salary"}
    );
    if(employees.has_value()) {
      for(auto e : employees.value()) {
        print_record(std::forward<decltype(e)>(e));
      }
      std::cout << '\n';
    } else {
      std::cout << "Error: " << employees.error().message() << '\n';
    }
  }
  
  // selecting data from specific columns
  {
    auto employees = employees_query
      .select<int, std::optional<std::string>, std::string, decimal<8,2>>(
        {"employee_id", "first_name", "last_name", "salary * 1.05 AS new_salary"}
    );
    if(employees.has_value()) {
      for(auto e : employees.value()) {
        print_record(std::forward<decltype(e)>(e));
      }
      std::cout << '\n';
    } else {
      std::cout << "Error: " << employees.error().message() << '\n';
    }
  }

  // count all records
  {
    auto count = employees_query
      .count();
    if(count.has_value()) {
      std::cout << "Employees Count(*): " << count.value() << '\n';
    } else {
      std::cout << "Error: " << count.error().message() << '\n';
    }
  }

  // count records by column
  {
    auto count = employees_query
      .count("phone_number");
    if(count.has_value()) {
      std::cout << "Employees Count(phone_number): " << count.value() << '\n';
    } else {
      std::cout << "Error: " << count.error().message() << '\n';
    }
  }

  // advanced count
  {
    std::cout << "SELECT department_id, COUNT(*) FROM employees GROUP BY department_id'\n";
    auto count = employees_query
      .group_by("department_id")
      .select<int, int>({"department_id", "COUNT(*)"});
    if(count.has_value()) {
      for(auto e : count.value()) {
        print_record(std::forward<decltype(e)>(e));
      }
      std::cout << '\n';
    } else {
      std::cout << "Error: " << count.error().message() << '\n';
    }
  }
  return EXIT_SUCCESS;
}
