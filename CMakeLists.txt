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
    # Some stuff from python-config, not ready yet for usage
    EXECUTE_PROCESS(COMMAND ${CUSTOM_PYTHON} --prefix  OUTPUT_VARIABLE PYTHON_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    EXECUTE_PROCESS(COMMAND ${CUSTOM_PYTHON} --ldflags OUTPUT_VARIABLE PYTHON_CONF_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    EXECUTE_PROCESS(COMMAND ${CUSTOM_PYTHON} --cflags OUTPUT_VARIABLE PYTHON_CONF_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    EXECUTE_PROCESS(COMMAND ${CUSTOM_PYTHON} --libs OUTPUT_VARIABLE PYTHON_CONF_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

    string(REGEX MATCH "[-][L]([^ ;])+" PYTHON_LINK_DIR ${PYTHON_CONF_LDFLAGS}) 
    string(REPLACE "-L" "" PYTHON_LINK_DIR ${PYTHON_LINK_DIR})

    string(REGEX MATCH "[-][I]([^ ;])+" PYTHON_INCLUDE_PATH ${PYTHON_CONF_CFLAGS})
    string(REPLACE "-I" "" PYTHON_INCLUDE_PATH ${PYTHON_INCLUDE_PATH})

    string(REGEX MATCHALL "[-][l]([^;])+" PYTHON_LIBRARY ${PYTHON_CONF_LDFLAGS})

    link_directories(${PYTHON_LINK_DIR})
ELSE (CUSTOM_PYTHON)
    FIND_PACKAGE(PythonLibs)
ENDIF (CUSTOM_PYTHON)

MESSAGE("python env:
\t Prefix: ${PYTHON_PREFIX} 
\t Lib dir: ${PYTHON_LINK_DIR}
\t Inludes dir: ${PYTHON_INCLUDE_PATH}
\t Libs2link: ${PYTHON_LIBRARY}")

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
