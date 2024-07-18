cmake_minimum_required(VERSION 3.5)

project(multi_file)

# setting constants
set(CMAKE_CXX_STANDARD 20)

cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)

if("${_host_name}" MATCHES "spmln|node0[0-9]")
  MESSAGE("Setting up for spmln")
  set(CMAKE_CXX_COMPILER "/home/s.ianniciello/.local/opt/gcc-13.2.0/bin/g++")
  set(CMAKE_CXX_FLAGS "-Wall -pthread -I/opt/ohpc/pub/mpi/openmpi5-gnu12/5.0.3/include -L/opt/ohpc/pub/mpi/openmpi5-gnu12/5.0.3/lib -Wl,-rpath -Wl,/opt/ohpc/pub/mpi/openmpi5-gnu12/5.0.3/lib -Wl,--enable-new-dtags -lmpi")
  add_definitions(-DDEBUG)
elseif("${_host_name}" STREQUAL "r7425renaissance")
  MESSAGE("Setting up for r7425renaissance")
  set(CMAKE_CXX_COMPILER "/home/s.ianniciello/.local/opt/gcc-13.2.0/bin/g++")
  set(CMAKE_CXX_FLAGS "-Wall -pthread")
  add_definitions(-DDEBUG)
else()
  MESSAGE("Setting up for others")
  set(CMAKE_CXX_FLAGS "-Wall -pthread -g")
  # add debug flag
  add_definitions(-DDEBUG)
endif()

include_directories(include/miniz include/logger include/fastflow)


add_subdirectory(src)
