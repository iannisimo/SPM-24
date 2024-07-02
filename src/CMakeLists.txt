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
add_executable(mpi mpi_main.cpp ../include/miniz/miniz.c)
find_package(MPI REQUIRED)
# set(CMAKE_C_COMPILER mpicxx)
target_link_libraries(mpi argparse utils mpi_lib MPI::MPI_CXX)
target_compile_definitions(mpi INTERFACE USE_MPI PARALLEL)

# find_package(MPI REQUIRED)

# target_include_directories(SeisSol-common-properties SYSTEM INTERFACE ${MPI_CXX_INCLUDE_PATH})
# target_link_libraries(SeisSol-common-properties INTERFACE MPI::MPI_C)

# target_compile_definitions(SeisSol-common-properties INTERFACE USE_MPI PARALLEL)