# adding the subdirectory containing source files needed to create *sum* object.
add_subdirectory("cui")
add_subdirectory("compressor")

# creating *main* object
add_executable(out main.cpp ../include/miniz/miniz.c)

# linking *cui* object to the *main* object to create final executable object *out*
target_link_libraries(out argparse compressor sequential)