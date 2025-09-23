option(SQLINQ_ENABLE_SANITIZERS "Enable sanitizers" OFF)

if(SQLINQ_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set(SANITIZER_FLAGS "-fsanitize=address,undefined")
  add_compile_options(${SANITIZER_FLAGS} -fno-omit-frame-pointer)
  add_link_options(${SANITIZER_FLAGS})
endif()

