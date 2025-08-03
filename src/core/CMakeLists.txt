add_library(sqlinq-core
  ${CMAKE_CURRENT_SOURCE_DIR}/datetime.cpp
)

target_include_directories(sqlinq-core PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(sqlinq-core PUBLIC cxx_std_20)

target_compile_options(sqlinq-core PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
                                   -Wunused -Wuninitialized>
)
