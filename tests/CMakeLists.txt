find_package(GTest)

if (NOT GTest_FOUND)
  message(STATUS "GTest not found, downloading from repository...")
  include(FetchContent)
  FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
  )
  FetchContent_MakeAvailable(GTest)
  find_package(GTest)
endif (NOT GTest_FOUND)

include_directories(${GTEST_INCLUDE_DIRS})
link_libraries(GTest::gtest GTest::gtest_main GTest::gmock)

set(UNIT_TEST_SOURCES
  backend/intermediate_storage_test.cpp
  core/db_result_test.cpp
  types/datetime_test.cpp
)

if(SQLINQ_USE_MYSQL)
  list(APPEND UNIT_TEST_SOURCES backend/mysql_backend_test.cpp)
endif()

if(SQLINQ_USE_SQLITE)
  list(APPEND UNIT_TEST_SOURCES backend/sqlite_backend_test.cpp)
endif()

foreach(TEST_SOURCE ${UNIT_TEST_SOURCES})
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

  add_executable(${TEST_NAME} ${TEST_SOURCE})
  target_link_libraries(${TEST_NAME} PRIVATE sqlinq)
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

