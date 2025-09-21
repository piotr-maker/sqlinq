# SQLinq - SQL Query Library in C++

SQLinq is a modern C++ library that enables easy and type-safe integration with SQL databases.
It takes inspiration from LINQ in C# and functional programming, while remaining close to SQL.
With SQLinq, you can:
- Build queries with a fluent LINQ-like API
- Use it as a lightweight ORM with create, find, get_all, update, remove
- Write aggregate queries (count, sum, avg, …)
- Stay type-safe thanks to lambdas and member pointers (no raw SQL strings)

## Defining tables

SQLinq gives you full control over how your C++ structs map to SQL tables.
This is done by specializing the `sqlinq::Table<Entity>` template and providing a `meta()` function with column definitions.

### Example

```cpp
struct User {
  int id;
  std::string name;
  int age;
};

template<> struct sqlinq::Table<User> {
  SQLINQ_COLUMN(0, User, id)
  SQLINQ_COLUMN(1, User, name)
  SQLINQ_COLUMN(2, User, age)

  static consteval auto meta() {
    using namespace sqlinq;
    return make_table<User>(
      "users",
      SQLINQ_COLUMN_META(User, id, "user_id")
          .primary_key()
          .autoincrement(),
      SQLINQ_COLUMN_META(User, name, "user_name"),
      SQLINQ_COLUMN_META(User, age, "user_age")
    );
  }
}

```
This approach is explicit and flexible:
- You can decide how struct fields map to SQL column name.
- You can declare primary_key, autoincrement, unique and more.
- In future metadata can also be used for automatic schema generation.
## Usage

### ORM-style (CRUD operations)
```cpp
int main() {
  SQLiteBackend backend;
  backend.connect("database.db");

  Database db{backend};

  // Create new record
  User u{ .id = 0, .name = "Alice", .age = 30};
  db.create(u);

  // Find by primary key
  auto found = db.find<User>(u.id);

  // Get all users
  auto users = db.get_all<User>();

  // Update
  if (found) {
    found->age = 31;
    db.update(*found);
  }

  // Delete
  db.remove<User>(u.id);
}
```

### LINQ-style (fluent queries)
```cpp
int main() {
  SQLiteBackend sqlite;
  sqlite.connect("database.db");

  Database db{backend};
  auto q = Query<User>()
        .select_all()
        .order_by(&User::name)
        .where([](const auto &user) { return user.age > 18; });
  for (auto &user : db.execute(q)) {
    std::cout << user.id << ' ' << user.name << '\n';
  }
}
```
### Aggregates
```cpp
int main() {
  SQLiteBackend sqlite;
  sqlite.connect("database.db");

  int64_t user_count;
  Database db{backend};
  auto q = Query<User>().select(count(&User::id));
  auto cursor = db.execute(q);
  if (cursor.next()) {
    user_count = std::get<0>(cursor.current());
  }
}
```
## Roadmap
Planned features and improvements for SQLinq:

- [x] Type-safe query API (lambdas, member pointers instead of raw SQL strings)
- [x] Support for both ORM-style and LINQ-style usage
- [x] Aggregate functions (`COUNT`)
- [x] Column constraints: `primary_key`, `autoincrement`, `unique`

### Short-term
- [ ] Aggregate functions (`SUM`, `AVG`, `MIN`, `MAX`)
- [ ] Join support via `include<Entity>()`
- [ ] Schema generation from C++ metadata (`meta()`) → auto-create SQL DDL
- [ ] Introduce `fetch_one()` and `fetch_optional()` for easier single-record queries
- [ ] Better error handling with dedicated exception hierarchy
- [ ] Expand unit test coverage to the full codebase (integration + edge cases)

### Mid-term
- [ ] Support for more SQL dialects (PostgreSQL backend)
- [ ] Composite primary keys
- [ ] Default values for columns (`default()`)
- [ ] Optimize query execution to minimize copies (move semantics everywhere)
- [ ] Improve developer ergonomics (clearer error messages, better compile-time diagnostics)

### Long-term
- [ ] Migration support (auto-generate `ALTER TABLE` from metadata changes)
- [ ] Code generation tooling (e.g. generate C++ structs from existing DB schema)
- [ ] Async database operations (C++20 coroutines)
- [ ] Connection pooling
- [ ] Benchmarking & performance tuning

## Requirements
* C++20 or higher
* CMake version 3.16 or higher
* SQL database development package installed (MySQL, SQLite3)
  _If SQLite3 is not installed system-wide, the library will automatically download and build it using CMake FetchContent_

## License
SQLinq is available under the MIT license.
