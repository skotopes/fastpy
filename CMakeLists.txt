cmake_minimum_required(VERSION 2.0)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(robot CXX)
INCLUDE(CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX("fcgiapp.h" HAVE_FCGI)

IF (NOT HAVE_FCGI)
    MESSAGE(FATAL_ERROR "Could not find fastcgi headers")
ENDIF(NOT HAVE_FCGI)

IF (CUSTOM_PYTHON)
    EXECUTE_PROCESS(COMMAND CUSTOM_PYTHON --prefix OUTPUT_VARIABLE PYTHON_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    EXECUTE_PROCESS(COMMAND CUSTOM_PYTHON --includes OUTPUT_VARIABLE PYTHON_INCLUDE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    EXECUTE_PROCESS(COMMAND CUSTOM_PYTHON --libs OUTPUT_VARIABLE PYTHON_LIBRARY OUTPUT_STRIP_TRAILING_WHITESPACE)    
ELSE (CUSTOM_PYTHON)
    FIND_PACKAGE(PythonLibs)
ENDIF (CUSTOM_PYTHON)

set(CMAKE_INSTALL_PREFIX "/usr")
add_subdirectory ( src )
include_directories ( src ${PYTHON_INCLUDE_PATH})

set(fastpy_src
    src/fastpy.cpp
    src/fp_config.cpp
    src/fp_fastcgi.cpp
    src/fp_log.cpp
    src/fp_ipc.cpp
    src/fp_worker.cpp
    src/fp_handler.cpp
    src/fp_helpers.cpp
)

add_executable (fastpy ${fastpy_src})

target_link_libraries(fastpy pthread fcgi ${PYTHON_LIBRARY})
set_target_properties(fastpy PROPERTIES VERSION 0.0.3 SOVERSION 0)

########### install files ###############

install(TARGETS fastpy RUNTIME DESTINATION bin)
