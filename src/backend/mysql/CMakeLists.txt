if (UNIX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(MYSQLCLIENT QUIET mysqlclient)
  
  if (MYSQLCLIENT_FOUND)
    message(STATUS "Found MySQL client: ${MYSQLCLIENT_LIBRARIES}")
    include_directories(${MYSQLCLIENT_INCLUDE_DIRS})
    link_libraries(${MYSQLCLIENT_LIBRARIES})
  else()
    # Spróbuj MariaDB Connector/C
    pkg_check_modules(MARIADB QUIET mariadb)

    if (MARIADB_FOUND)
      message(STATUS "Found MariaDB connector: ${MARIADB_LIBRARIES}")
      include_directories(${MARIADB_INCLUDE_DIRS})
      link_libraries(${MARIADB_LIBRARIES})
    else()
      message(FATAL_ERROR
        "Neither MySQL client library nor MariaDB connector found.\n"
        "Please install libmysqlclient-dev or libmariadb-dev.")
    endif()
  endif()
else()
  find_package(unofficial-mysql-connector-c QUIET)
  if (unofficial-mysql-connector-c_FOUND)
    message(STATUS "Found MySQL (via vcpkg)")
    include_directories(${unofficial-mysql-connector-c_INCLUDE_DIRS})
    link_libraries(${unofficial-mysql-connector-c_LIBRARIES})
  else()
    message(FATAL_ERROR
      "MySQL client not found. Please install via vcpkg:\n"
      "vcpkg install mysql-connector-c")
  endif()
endif()

add_library(mysql-backend
  mysql_backend.cpp
)

target_include_directories(mysql-backend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(mysql-backend
  sqlinq-core
)

target_compile_options(mysql-backend PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
                                   -Wunused -Wuninitialized>
)
