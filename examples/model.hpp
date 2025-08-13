#ifndef EXAMPLES_MODEL_HPP
#define EXAMPLES_MODEL_HPP

#include <ctime>
#include <string>
#include <optional>
#include <sqlinq/types.h>
#include <sqlinq/detail/column.hpp>

struct Regions {
  int region_id;
  std::string region_name;
};
/*SQLINQ_COLUMNS(Regions, region_id, region_name)*/

struct Countries {
  int country_id;
  std::optional<std::string> country_name;
  int region_id; /* Regions region */
};

struct Locations {
  int location_id;
  std::optional<std::string> street_address;
  std::optional<std::string> postal_code;
  std::string city;
  std::optional<std::string> state_province;
  std::string country_id; /* Countries country */
};

struct Jobs {
  int job_id;
  std::string job_title;
  std::optional<sqlinq::decimal<8, 2>> min_salary;
  std::optional<sqlinq::decimal<8, 2>> max_salary;
};

struct Departments {
  int department_id;
  std::string department_name;
  std::optional<int> location_id; /* Locations location */
};

struct Employees {
  int employee_id;
  std::optional<std::string> first_name;
  std::string last_name;
  std::string email;
  std::optional<std::string> phone_number;
  std::time_t hire_date;
  int job_id; /* Jobs jobs */
  sqlinq::decimal<8, 2> salary;
  std::optional<int> manager_id; /* Employees employee */
  std::optional<int> department_id; /* Departments department */
};

struct Dependents {
  int dependent_id;
  std::string first_name;
  std::string last_name;
  std::string relationship;
  int employee_id; /* Employeed employee */
};

#endif /* EXAMPLES_MODEL_HPP */
