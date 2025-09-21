find_package(SQLite3 QUIET)

if (NOT SQLite3_FOUND)
  include(FetchContent)
  message(STATUS "SQLite3 not found system-wide, fetching with FetchContent...")
  FetchContent_Declare(
    sqlite3_amalgamation
    URL https://sqlite.org/2025/sqlite-amalgamation-3500400.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(sqlite3_amalgamation)

  add_library(SQLite3 STATIC ${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c)
  target_include_directories(SQLite3 PUBLIC ${sqlite3_amalgamation_SOURCE_DIR})
else()
  message(STATUS "Using system SQLite3: ${SQLite3_INCLUDE_DIRS}")
  add_library(SQLite3 INTERFACE)
  target_include_directories(SQLite3 INTERFACE ${SQLite3_INCLUDE_DIRS})
  target_link_libraries(SQLite3 INTERFACE ${SQLite3_LIBRARIES})
endif()

add_library(sqlite-backend
  sqlite_backend.cpp
)

target_include_directories(sqlite-backend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sqlite-backend
  sqlinq-core
  SQLite3
)

target_compile_options(sqlite-backend PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
                                   -Wunused -Wuninitialized>
)
