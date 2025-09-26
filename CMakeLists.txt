cmake_minimum_required(VERSION 3.16)
project(sqlinq VERSION 0.2.1)
#configure_file(config.hpp.in include/sqlinq/config.hpp @ONLY)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(cmake/sanitizers.cmake)
option(SQLINQ_BUILD_TESTS "Build tests" OFF)
option(SQLINQ_BUILD_EXAMPLES "Build examples" OFF)

# Backend options
option(SQLINQ_USE_SQLITE "Enable SQLite backend" ON)
option(SQLINQ_USE_MYSQL "Enable MySQL backend" OFF)

if(SQLINQ_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(SQLINQ_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

set(SQLINQ_BACKEND_LIBS "")
if(SQLINQ_USE_MYSQL)
  add_subdirectory(src/backend/mysql)
  list(APPEND SQLINQ_BACKEND_LIBS mysql-backend)
endif()

if(SQLINQ_USE_SQLITE)
  add_subdirectory(src/backend/sqlite)
  list(APPEND SQLINQ_BACKEND_LIBS sqlite-backend)
endif()

add_subdirectory(src/core)
add_library(sqlinq INTERFACE)
target_include_directories(sqlinq INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(sqlinq INTERFACE
  sqlinq-core
  ${SQLINQ_BACKEND_LIBS}
)
