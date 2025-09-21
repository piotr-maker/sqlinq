cmake_minimum_required(VERSION 3.16)
project(examples)

link_libraries(sqlinq)

# Basic examples (require SQLite)
if (SQLINQ_USE_SQLITE)
  set(SQLITE_DB_FILE "${CMAKE_CURRENT_BINARY_DIR}/personnel.sqlite3")
  set(CREATE_SQL "${CMAKE_CURRENT_SOURCE_DIR}/sql/sqlite/create_tables.sql")
  set(INSERT_SQL "${CMAKE_CURRENT_SOURCE_DIR}/sql/sqlite/insert.sql")

  add_custom_command(
    OUTPUT ${SQLITE_DB_FILE}
    COMMAND sqlite3 ${SQLITE_DB_FILE} < ${CREATE_SQL}
    COMMAND sqlite3 ${SQLITE_DB_FILE} < ${INSERT_SQL}
    DEPENDS ${CREATE_SQL} ${INSERT_SQL}
    COMMENT "Generating SQLite database: ${SQLITE_DB_FILE}"
    VERBATIM
  )
  add_custom_target(generate_sqlite ALL DEPENDS ${SQLITE_DB_FILE})

  add_executable(basic_aggregate src/basic_aggregate_example.cpp)
  add_dependencies(basic_aggregate generate_sqlite)
  target_compile_definitions(basic_aggregate PRIVATE SQLITE_DB_FILE="${SQLITE_DB_FILE}")
  target_compile_options(basic_aggregate PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
                                   -Wunused -Wuninitialized>
  )

  add_executable(basic_orm src/basic_orm_example.cpp)
  add_dependencies(basic_orm generate_sqlite)
  target_compile_definitions(basic_orm PRIVATE SQLITE_DB_FILE="${SQLITE_DB_FILE}")
  target_compile_options(basic_orm PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
                                   -Wunused -Wuninitialized>
  )
endif()

# Advanced examples (require MySQL)
if (SQLINQ_USE_MYSQL)
  add_executable(advanced src/advanced_example.cpp)
  target_compile_options(advanced PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
                                   -Wunused -Wuninitialized>
  )
endif()
