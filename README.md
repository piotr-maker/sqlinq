# SQLinq - SQL Query Library in C++

SQLinq is a C++ library that enables easy and efficient integration with SQL databases, using a syntax similar to LINQ from C#. With SQLinq, you can write database queries in a way that's closer to functional programming, simplifying code and improving readability.

## Table naming convention
In SQLinq, the names of tables are automatically generated from structure or class name used in the code. The library follows a naming convention where:
* The class name is converted to lowercase.
* The name of table is based on the structure name

For example, if you have C++ structure like this:
```
struct User {
	int id;
	std::string name;
	int age;
};
```
The corresponding table name in SQL will be `user`.
This ensures that the mapping between C++ classes and SQL tables is consistent and follows common naming conventions.

## Usage

### Example
Select in SQLinq can be used in two ways:
1) getting whole structure:
```
#include <cstdlib>
#include <iostream>
#include <sqlinq/query.hpp>
#include <sqlinq/sqlite/database.h>

using namespace sqlinq;

struct User {
  int id;
  std::string name;
  int age;
};

int main() {
  sqlite::database db;
  db.connect("database.db");

  query<User> user_query;
  std::vector<User> users;
  try {
    users = user_query
      .where("age > 18")
      .order_by("name")
      .select();
  } catch(const exception& ex) {
    std::cout << ex.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
```
2) getting specific columns:
```
#include <cstdlib>
#include <iostream>
#include <sqlinq/query.hpp>
#include <sqlinq/sqlite/database.h>

using namespace sqlinq;

struct User {
  int id;
  std::string name;
  int age;
};

int main() {
  sqlite::database db;
  db.connect("database.db");

  query<User> user_query;
  std::vector<std::tuple<int, std::string>> records;
  try {
    records = user_query
      .where("age > 18")
      .order_by("age")
      .select<int, std::string>("age", "name");
  } catch(const exception& ex) {
    std::cout << ex.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
```

## Requirements
* C++20 or higher
* CMake version 3.16 or higher
* SQL database development package installed (MySQL, SQLite3)

## License
SQLinq is available under the MIT license.
