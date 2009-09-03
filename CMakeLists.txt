cmake_minimum_required(VERSION 2.0)
project(robot CXX)
INCLUDE(CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX("fcgiapp.h" HAVE_FCGI)

IF (NOT HAVE_FCGI)
    MESSAGE(FATAL_ERROR "Could not find fastcgi headers")
ENDIF(NOT HAVE_FCGI)

find_package(PythonLibs)

set(CMAKE_INSTALL_PREFIX "/usr")
add_subdirectory ( src )
include_directories ( src ${PYTHON_INCLUDE_PATH})

set(fastpy_src
    src/fastpy.cpp
    src/fp_config.cpp
    src/fp_fastcgi.cpp
    src/fp_core.cpp
    src/fp_log.cpp
    src/fp_worker.cpp
)

add_executable (fastpy ${fastpy_src})

target_link_libraries(fastpy fcgi ${PYTHON_LIBRARY})
set_target_properties(fastpy PROPERTIES VERSION 0.0.1 SOVERSION 0)

########### install files ###############

install(TARGETS fastpy RUNTIME DESTINATION bin)
