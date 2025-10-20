# SQLinq Table Definitions

SQLinq lets you define SQL table mappings in two ways:

1. **Manual metadata definitions** — full control, explicit code.
2. **Automatic generation** — use C++ attributes and let SQLinq generate everything for you.

---

## Manual table definitions
Manual table mapping gives you precise control over table and column behavior.
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
};
```

## Automatic table generation
Alternatively, SQLinq can generate all `sqlinq::Table<T>` specializations automatically.
Just annotate your structs:
```cpp
struct [[table("comments")]] Comment {
  [[name("comment_id"), autoincrement, primary_key]]
  int id;

  [[foreign_key("posts.post_id")]]
  int post_id;

  std::string author;
  std::string content;
};
```

### Requirements
- Must be defined in a **header file** (.hpp or .h)
- Must include the [[table("name")]] attribute
- May include column and constraint attributes:
  - `[[name("column_name")]]`
  - `primary_key`, `autoincrement`, `unique`
  - `foreign_key("table.column")`

### Using the generated schema
After building your project, include the generated file:
```cpp
#include "table_schema.hpp"
```

### How it works
When you call `sqlinq_init_venv()` in your `CMakeLists.txt`:
1) CMake checks if a local Python virtual environment exists under the build directory ($`{CMAKE_BINARY_DIR}/.venv`).
2) If the environment does not exist:
- A new virtual environment is created.
- Required Python packages are installed automatically:
    - `tree-sitter`
    - `tree-sitter-cpp`
    - `tree-sitter-languages`
3) The path to the Python executable inside the venv is stored in `SQLINQ_PYTHON_EXECUTABLE` for later use by `sqlinq_generate_schema()`.
4) Subsequent calls to `sqlinq_generate_schema()` will use this venv automatically, ensuring that all dependencies are available without requiring system-wide Python package installation.
**Note**: The virtual environment is created only once per build directory.
This keeps the build reproducible and avoids modifying the global Python environment.

When `sqlinq_generate_schema()` is added to your `CMakeLists.txt`:
1) CMake scans all targets in the project.
2) For each target linked with sqlinq:
    - Gathers its include directories.
    - Searches for headers defining `[[table(...)]]` structs.
3) Runs the Python generator (`cpp2schema.py`) to produce:
    - `generated/include/table_schema.hpp`
	- `generated/schema.my.hcl`
	- `generated/schema.lt.hcl`

The generated files are automatically refreshed whenever headers change.

This system is designed with **C++26 static reflection** in mind —
in future, attributes will be read directly by the compiler, eliminating generation steps.

