function(sqlinq_init_venv)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  if(WIN32)
    set(_venv_python "${CMAKE_BINARY_DIR}/.venv/Scripts/python.exe")
  else()
    set(_venv_python "${CMAKE_BINARY_DIR}/.venv/bin/python3")
  endif()

  if(EXISTS "${_venv_python}")
    message(STATUS "sqlinq_init_venv: Using existing Python virtual environment at ${CMAKE_BINARY_DIR}/.venv")
  else()
    message(STATUS "sqlinq_init_venv: Creating local Python virtual environment...")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m venv ${CMAKE_BINARY_DIR}/.venv
        RESULT_VARIABLE _venv_result
    )
    if(NOT _venv_result EQUAL 0)
      message(FATAL_ERROR "sqlinq_init_venv: Failed to create Python virtual environment.")
    endif()

    message(STATUS "sqlinq_init_venv: Installing Python dependencies (tree-sitter)...")
    execute_process(
        COMMAND "${_venv_python}" -m pip install --upgrade pip
    )
    execute_process(
        COMMAND "${_venv_python}" -m pip install
          tree-sitter==0.25.2
          tree-sitter-cpp==0.23.4
          tree-sitter-languages==1.10.2
        RESULT_VARIABLE _pip_result
    )
    if(NOT _pip_result EQUAL 0)
      message(FATAL_ERROR "sqlinq_init_venv: Failed to install Python dependencies.")
    endif()
  endif()

  set(SQLINQ_PYTHON_EXECUTABLE "${_venv_python}" PARENT_SCOPE)
  message(STATUS "sqlinq_init_venv: Python executable set to ${_venv_python}")
endfunction()

