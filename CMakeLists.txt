cmake_minimum_required(VERSION 3.16)
project(sqlinq VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(SQLite3 REQUIRED)
pkg_check_modules(MYSQL_CLIENT REQUIRED mysqlclient)
option(SQLINQ_BUILD_EXAMPLES "Build examples" OFF)

if(SQLINQ_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

add_library(${PROJECT_NAME}
  src/mysql/error.cpp
  src/sqlite/database.cpp
  src/sqlite/error.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${MYSQL_CLIENT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${SQLite3_LIBRARIES}
  ${MYSQL_CLIENT_LIBRARIES}
)
