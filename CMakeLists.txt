cmake_minimum_required(VERSION 2.6)
include(CheckIncludeFileCXX)
set(CMAKE_INSTALL_PREFIX "/usr")

project(robot CXX)
add_subdirectory(3rd_party/libev)

check_include_file_cxx("fcgiapp.h" HAVE_FCGI)

if (NOT HAVE_FCGI)
    message(FATAL_ERROR "Could not find fastcgi headers")
endif (NOT HAVE_FCGI)

if (CUSTOM_PYTHON)
    execute_process(COMMAND ${CUSTOM_PYTHON} --prefix  OUTPUT_VARIABLE PYTHON_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${CUSTOM_PYTHON} --ldflags OUTPUT_VARIABLE PYTHON_CONF_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${CUSTOM_PYTHON} --cflags OUTPUT_VARIABLE PYTHON_CONF_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${CUSTOM_PYTHON} --libs OUTPUT_VARIABLE PYTHON_CONF_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

    string(REGEX MATCH "[-][L]([^ ;])+" PYTHON_LINK_DIR ${PYTHON_CONF_LDFLAGS})

    if (NOT PYTHON_LINK_DIR)
        set(PYTHON_LINK_DIR "${PYTHON_PREFIX}/lib")
    else (NOT PYTHON_LINK_DIR)
        string(REPLACE "-L" "" PYTHON_LINK_DIR ${PYTHON_LINK_DIR})
    endif (NOT PYTHON_LINK_DIR)

    string(REGEX MATCH "[-][I]([^ ;])+" PYTHON_INCLUDE_PATH ${PYTHON_CONF_CFLAGS})
    string(REPLACE "-I" "" PYTHON_INCLUDE_PATH ${PYTHON_INCLUDE_PATH})
    string(REGEX MATCHALL "[-][l]([^;])+" PYTHON_LIBRARY ${PYTHON_CONF_LDFLAGS})

    link_directories(${PYTHON_LINK_DIR})
else (CUSTOM_PYTHON)
    find_package(PythonLibs)
endif (CUSTOM_PYTHON)

MESSAGE("Python build env:
 \t Prefix: ${PYTHON_PREFIX}
\t Lib dir: ${PYTHON_LINK_DIR}
\t Inludes dir: ${PYTHON_INCLUDE_PATH}
\t Libs2link: ${PYTHON_LIBRARY}")

include_directories( src 3rd_party/libev ${PYTHON_INCLUDE_PATH})

set(fastpy_src
    src/fastpy.cpp
    src/fp_config.cpp
    src/fp_fastcgi.cpp
    src/fp_log.cpp
    src/fp_ipc.cpp
    src/fp_thread.cpp
    src/fp_worker.cpp
    src/fp_handler.cpp
    src/fp_helpers.cpp
)

add_executable(fastpy ${fastpy_src})

target_link_libraries(fastpy ${PYTHON_LIBRARY} pthread fcgi ev rt)
set_target_properties(fastpy PROPERTIES VERSION 0.1.0 SOVERSION 0)

########### install files ###############

install(TARGETS fastpy RUNTIME DESTINATION bin)
