#ifndef EXAMPLES_MODEL_HPP
#define EXAMPLES_MODEL_HPP

#include <ctime>
#include <optional>
#include <sqlinq/column.hpp>
#include <sqlinq/detail/column.hpp>
#include <sqlinq/table.hpp>
#include <sqlinq/types.h>
#include <string>

struct Regions {
  int id;
  std::string name;
};

struct Countries {
  std::string id;
  std::string name;
  int region; /* Regions region */
};
template <> struct sqlinq::Table<Countries> {
  SQLINQ_COLUMN(0, Countries, id)
  SQLINQ_COLUMN(1, Countries, name)
  SQLINQ_COLUMN(2, Countries, region)

  static consteval auto meta() {
    using namespace sqlinq;
    return make_table<Countries>(
        "countries",
        SQLINQ_COLUMN_META(Countries, id, "country_id")
            .primary_key()
            .autoincrement(),
        SQLINQ_COLUMN_META(Countries, name, "country_name"),
        SQLINQ_COLUMN_META(Countries, region, "region_id"));
  }
};

struct Locations {
  int id;
  std::optional<std::string> street_address;
  std::optional<std::string> postal_code;
  std::string city;
  std::optional<std::string> state_province;
  std::string country; /* Countries country */
};

struct Jobs {
  int id;
  std::string title;
  sqlinq::Decimal<8, 2> min_salary;
  sqlinq::Decimal<8, 2> max_salary;
};
template <> struct sqlinq::Table<Jobs> {
  SQLINQ_COLUMN(0, Jobs, id)
  SQLINQ_COLUMN(1, Jobs, title)
  SQLINQ_COLUMN(2, Jobs, min_salary)
  SQLINQ_COLUMN(3, Jobs, max_salary)

  static consteval auto meta() {
    using namespace sqlinq;
    return make_table<Jobs>(
        "jobs",
        SQLINQ_COLUMN_META(Jobs, id, "job_id").autoincrement().primary_key(),
        SQLINQ_COLUMN_META(Jobs, title, "job_title"),
        SQLINQ_COLUMN_META(Jobs, min_salary, "min_salary"),
        SQLINQ_COLUMN_META(Jobs, max_salary, "max_salary"));
  }
};

struct Departments {
  int id;
  std::string name;
  int location; /* Locations location */
};

struct Employees {
  int id;
  std::optional<std::string> first_name;
  std::string last_name;
  std::string email;
  std::optional<std::string> phone_number;
  sqlinq::Date hire_date;
  int job_id; /* Jobs jobs */
  sqlinq::Decimal<8, 2> salary;
  std::optional<int> manager;    /* Employees employee */
  std::optional<int> department; /* Departments department */
};
template <> struct sqlinq::Table<Employees> {
  SQLINQ_COLUMN(0, Employees, id)
  SQLINQ_COLUMN(1, Employees, first_name)
  SQLINQ_COLUMN(2, Employees, last_name)
  SQLINQ_COLUMN(3, Employees, email)
  SQLINQ_COLUMN(4, Employees, phone_number)
  SQLINQ_COLUMN(5, Employees, hire_date)
  SQLINQ_COLUMN(6, Employees, job_id)
  SQLINQ_COLUMN(7, Employees, salary)
  SQLINQ_COLUMN(8, Employees, manager)
  SQLINQ_COLUMN(9, Employees, department)

  static consteval auto meta() {
    using namespace sqlinq;
    return make_table<Employees>(
        "employees",
        SQLINQ_COLUMN_META(Employees, id, "employee_id")
            .primary_key()
            .autoincrement(),
        SQLINQ_COLUMN_META(Employees, first_name, "first_name"),
        SQLINQ_COLUMN_META(Employees, last_name, "last_name"),
        SQLINQ_COLUMN_META(Employees, email, "email"),
        SQLINQ_COLUMN_META(Employees, phone_number, "phone_number"),
        SQLINQ_COLUMN_META(Employees, hire_date, "hire_date"),
        SQLINQ_COLUMN_META(Employees, job_id, "job_id"),
        SQLINQ_COLUMN_META(Employees, salary, "salary"),
        SQLINQ_COLUMN_META(Employees, manager, "manager_id"),
        SQLINQ_COLUMN_META(Employees, department, "department_id"));
  }
};

struct Dependents {
  int id;
  std::string first_name;
  std::string last_name;
  std::string relationship;
  int employee; /* Employeed employee */
};

#endif /* EXAMPLES_MODEL_HPP */
