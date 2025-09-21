# Database configuration

SQLinq supports configuration through **INI files** and **environment variables**.
This allows you to switch between backends (SQLite, MySQL) without recompiling your code.

## INI file format

An INI file consists of sections, one per backend
Example:

```ini
[sqlite]
database=personnel.sqlite3

[mysql]
host=localhost
port=3306
user=piotr
password=passwd
database=personnel
```

- Section name ([sqlite], [mysql]) matches the backend you want to use.
- Each key/value pair corresponds to a field in `DatabaseConfig`.

## Environment variables
You can set values using environment variables.
Variable name is constructed as:
```
SQLINQ_<SECTION>_<KEY>
```

## Usage in code
```cpp
#include <sqlinq/config.hpp>
#include <sqlinq/sqlite_backend.hpp>
#include <sqlinq/mysql_backend.hpp>

// load configuration from file
auto cfg = sqlinq::load_db_config("db.conf");

// SQLite
sqlinq::SQLiteBackend sqlite;
sqlite.connect(cfg.at("sqlite"));

// MySQL
sqlinq::MySQLBackend mysql;
mysql.connect(cfg.at("mysql"));
```
This approach lets you keep credentials out of source code and switch between databases easily.

## Security considerations
- Keep credentials out of source code – do not hardcode usernames, passwords, or file paths in your program.
- Prefer environment variables in production – they are easier to rotate and integrate with deployment tools (Docker, Kubernetes, CI/CD).
- INI files can be used for development – convenient for local testing, but remember to exclude them from version control (e.g. add to .gitignore).
- By externalizing credentials, you reduce the risk of accidentally leaking sensitive information in commits, binaries, or logs.
