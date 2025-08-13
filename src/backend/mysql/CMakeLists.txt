find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL_CLIENT REQUIRED mysqlclient)

add_library(mysql-backend
  mysql_backend.cpp
)

target_include_directories(mysql-backend PUBLIC
  ${MYSQL_CLIENT_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mysql-backend
  sqlinq-core
  ${MYSQL_CLIENT_LIBRARIES}
)

target_compile_options(mysql-backend PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
                                   -Wunused -Wuninitialized>
)
