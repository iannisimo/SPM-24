# creating object file *argparse* from the source code
add_library(compressor SHARED utils.cpp)
add_library(sequential SHARED sequential.cpp)
add_library(fastflow SHARED fastflow.cpp)