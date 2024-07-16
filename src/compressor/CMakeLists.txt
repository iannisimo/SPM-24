# creating object file *argparse* from the source code
add_library(utils SHARED utils.cpp)
add_library(sequential_lib SHARED sequential.cpp)
add_library(fastflow_lib SHARED fastflow.cpp)

cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)
if(NOT "${_host_name}" STREQUAL "r7425renaissance")
  add_library(mpi_lib SHARED mpi.cpp)
endif()