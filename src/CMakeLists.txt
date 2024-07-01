# adding the subdirectory containing source files needed to create *sum* object.
add_subdirectory("cui")
add_subdirectory("compressor")

# creating *sequential* object
add_executable(sequential main.cpp ../include/miniz/miniz.c)
set_target_properties(sequential PROPERTIES COMPILE_FLAGS "-DSEQUENTIAL")
target_link_libraries(sequential argparse utils sequential_lib)
# creating *fastflow* object
add_executable(fastflow main.cpp ../include/miniz/miniz.c)
set_target_properties(fastflow PROPERTIES COMPILE_FLAGS "-DFASTFLOW")
target_link_libraries(fastflow argparse utils fastflow_lib)
# # creating *mpi* object
# add_executable(mpi main.cpp ../include/miniz/miniz.c)
# set_target_properties(mpi PROPERTIES COMPILE_FLAGS "-DMPI")
# target_link_libraries(mpi argparse utils mpi_lib)