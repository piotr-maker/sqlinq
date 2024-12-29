cmake_minimum_required(VERSION 3.16)
project(sqlinq VERSION 0.1.0)
include(cmake/plugins.cmake)
configure_file(config.hpp.in include/sqlinq/config.hpp @ONLY)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(SQLINQ_BUILD_EXAMPLES "Build examples" OFF)
if(SQLINQ_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

add_library(${PROJECT_NAME}
  src/sqlite/database.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
  ${SQLINQ_PLUGIN_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${SQLINQ_PLUGIN_LIBRARIES}
)
