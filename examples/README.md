# SQLinq Examples
This directory contains examples demonstrating different ways to use SQLinq, from simple ORM-style operations to advanced LINQ-like queries and aggregate functions.
All examples are designed to be self-contained, with SQLite databases automatically prepared for the basic examples. For MySQL (advanced examples), you need a running server and provided SQL scripts.

## Preparing the database
Before running the examples, you need to ensure that the databases are available.

### SQLite (Basic examples)
The SQLite database is automatically generated during the build:
- A fresh personnel.sqlite3 file is created in the build directory.
- create_table.sql and insert.sql located in examples/sql/ are executed automatically.
This allows you to run the basic examples immediately after building:

### MySQL (Advanced examples)
For advanced examples using MySQL:
- Start a MySQL server (e.g. with Docker):
```sh
docker run --name sqlinq-mysql \
        -e MYSQL_ROOT_PASSWORD=passwd \
        -e MYSQL_DATABASE=personnel \
        -p 3306:3306 \
        -d mysql:8
```
- Apply the schema and sample data:
```sh
mysql -h 127.0.0.1 -u root -p personnel < examples/sql/mysql/create_tables.sql
mysql -h 127.0.0.1 -u root -p personnel < examples/sql/mysql/insert.sql
# Enter password: passwd
```
**Tip:** Use 127.0.0.1 instead of localhost to ensure TCP connection instead of Unix socket. 

## Basic Examples
These examples show simple ORM-style usage with SQLite:
- `src/basic_orm.cpp` – simple ORM style: create, find, get_all, update, remove
- `src/basic_aggregate.cpp` – aggregate functions like `count()`

### How to run
1. Make sure you have SQLite3 development package installed.
2. Build the project with CMake:
```sh
   cmake -DSQLINQ_BUILD_EXAMPLES=ON -B build .
   cd build
   make
```
3. Run the examples:
```sh
./examples/basic_orm
./examples/basic_aggregate
```

## Advanced Examples
These examples demonstrate more advanced SQLinq usage, including:
- LINQ-style query building with multiple clauses
- Combining ORM-style with fluent queries
- Executing aggregate functions (COUNT, SUM, AVG, …)
- (Planned) JOIN queries with .include<Entity>()

### Requirements:
- MySQL server running and accessible (configured in db.config)
- Tables must exist before running the examples (see examples/sql/mysql/create_tables.sql and insert.sql)

### How to run
1. Make sure you have MySQL development package installed.
2. Build the project with CMake:
```sh
   cmake -DSQLINQ_BUILD_EXAMPLES=ON -DSQLINQ_USE_MYSQL=ON -B build .
   cd build
   make
```
3. Run the examples:
```sh
./examples/advanced
```

## Notes
- All examples are type-safe and leverage member pointers and lambdas instead of raw SQL strings.
- SQLite examples can run immediately without additional configuration.
- MySQL examples require a running server and proper credentials in `db.config`
- Both SQLite and MySQL examples use the same schema to ensure compatibility.
- For CI or automated testing, SQLite examples provide a lightweight and fast test environment.
