function(sqlinq_generate_schema_auto)
  if(NOT DEFINED SQLINQ_PYTHON_EXECUTABLE)
    message(FATAL_ERROR "sqlinq_generate_schema_auto: Python virtual environment not initialized. Call sqlinq_init_venv() first.")
  endif()

  #find_package(Python3 REQUIRED COMPONENTS Interpreter)
  cmake_parse_arguments(ARG "" "OUT_DIR" "" ${ARGN})

  if(NOT ARG_OUT_DIR)
    message(FATAL_ERROR "sqlinq_generate_schema_auto: OUT_DIR must be specified")
  endif()

  file(MAKE_DIRECTORY ${ARG_OUT_DIR})
  file(MAKE_DIRECTORY ${ARG_OUT_DIR}/include)
  get_property(all_targets DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)

  set(SQLINQ_TARGETS "")
  foreach(tgt IN LISTS all_targets)
    get_target_property(libs ${tgt} INTERFACE_LINK_LIBRARIES)
    if(NOT libs)
      get_target_property(libs ${tgt} LINK_LIBRARIES)
    endif()

    if(libs)
      foreach(lib IN LISTS libs)
        if(lib STREQUAL "sqlinq")
          list(APPEND SQLINQ_TARGETS ${tgt})
        endif()
      endforeach()
    endif()
  endforeach()

  if(NOT SQLINQ_TARGETS)
    message(WARNING "sqlinq_generate_schema_auto: no targets linking with sqlinq found")
    return()
  endif()

  set(SQLINQ_SCHEMA_INPUTS "")
  set(SQLINQ_INCLUDE_DIRS "")

  foreach(tgt IN LISTS SQLINQ_TARGETS)
    get_target_property(srcs ${tgt} SOURCES)
    get_target_property(includes ${tgt} INCLUDE_DIRECTORIES)

    # if(srcs)
    #     list(APPEND SQLINQ_SCHEMA_INPUTS ${srcs})
    # endif()

    if(includes)
      list(APPEND SQLINQ_INCLUDE_DIRS ${includes})
    endif()
  endforeach()

  file(GLOB_RECURSE SQLINQ_SCHEMA_INPUTS CONFIGURE_DEPENDS
    ${SQLINQ_INCLUDE_DIRS}/*.hpp
  )

  if(NOT SQLINQ_SCHEMA_INPUTS)
    message(WARNING "sqlinq_generate_schema_auto: no input files found")
    return()
  endif()

  list(REMOVE_DUPLICATES SQLINQ_INCLUDE_DIRS)

  # Prepare include args for Python script
  set(INCLUDE_ARGS "")
  foreach(inc IN LISTS SQLINQ_INCLUDE_DIRS)
    list(APPEND INCLUDE_ARGS --include ${inc})
  endforeach()

  # Prepare input args for Python script
  set(INPUT_ARGS "")
  foreach(inp IN LISTS SQLINQ_SCHEMA_INPUTS)
    list(APPEND INPUT_ARGS --input ${inp})
  endforeach()

  message(STATUS "sqlinq_generate_schema_auto: generating schema for ${SQLINQ_TARGETS}")

  get_target_property(SQLINQ_SOURCE_DIR sqlinq SOURCE_DIR)
  set(CPP2SCHEMA ${SQLINQ_SOURCE_DIR}/tools/cpp2schema.py)
  set(GENERATED_HPP ${ARG_OUT_DIR}/include/table_schema.hpp)
  set(GENERATED_MY_HCL ${ARG_OUT_DIR}/schema.my.hcl)
  set(GENERATED_LT_HCL ${ARG_OUT_DIR}/schema.lt.hcl)
  message(WARNING "SQLINQ_SCHEMA_NAME ${SQLINQ_SCHEMA_NAME}")

  add_custom_command(
    OUTPUT ${GENERATED_HPP} ${GENERATED_MY_HCL} ${GENERATED_LT_HCL}
    COMMAND ${SQLINQ_PYTHON_EXECUTABLE} ${CPP2SCHEMA}
          ${INPUT_ARGS}
          --outdir ${ARG_OUT_DIR}
          --schema-name ${SQLINQ_SCHEMA_NAME}
    DEPENDS ${CPP2SCHEMA}
    COMMENT "Generating schema from C++ structs"
    VERBATIM
  )

  add_custom_target(sqlinq-schema ALL
    DEPENDS ${GENERATED_HPP} ${GENERATED_MY_HCL} ${GENERATED_LT_HCL}
  )

  include_directories(${ARG_OUT_DIR}/include)
endfunction()

