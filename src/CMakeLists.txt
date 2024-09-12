# adding the subdirectory containing source files needed to create *sum* object.
add_subdirectory("cui")
add_subdirectory("compressor")


# creating *sequential* object
add_executable(sequential s_main.cpp ../include/miniz/miniz.c)
target_link_libraries(sequential argparse utils sequential_lib)
# creating *fastflow* object
add_executable(fastflow ff_main.cpp ../include/miniz/miniz.c)
target_link_libraries(fastflow argparse utils fastflow_lib)
# # creating *mpi* object

cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)
if(NOT "${_host_name}" STREQUAL "r7425renaissance")
  find_package(MPI)
  add_executable(mpi mpi_main.cpp ../include/miniz/miniz.c)
  target_link_libraries(mpi argparse utils mpi_lib MPI::MPI_CXX)
  target_compile_definitions(mpi INTERFACE USE_MPI PARALLEL)
endif()
