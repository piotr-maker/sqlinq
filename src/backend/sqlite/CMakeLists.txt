find_package(SQLite3 REQUIRED)

add_library(sqlite-backend
  sqlite_backend.cpp
)

target_include_directories(sqlite-backend PUBLIC
  ${SQLite3_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sqlite-backend
  sqlinq-core
  ${SQLite3_LIBRARIES}
)

target_compile_options(sqlite-backend PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
                                   -Wunused -Wuninitialized>
)
