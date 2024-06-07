Parallel and Distributed Systems: Paradigms and Models
======================================================
Project for the exam session of A.Y. 23/24
------------------------------------------

### Project Specification:

#### Distributed ad-hoc compressor/decompressor using Miniz
Following the idea presented in the __FFC__ example code provided in the spmcode7.zip tarball, implement a distributed file compressor in __MPI__ capable of compressing/decrompressing all files contained in a given directory with the following constraints:

  1. Only one process reads the file to compress/decompress from the filesystem.
  2. It is not possible to use temporary files for the computation nor spawning service processes (e.g., tar, zip, cat, size, …)
  3. The compression and decompression must use the Miniz library, as exemplified in the FFC example.

Provide also a shared-memory version of the compressor/decompressor using FastFlow that uses only pipeline and all-to-all parallel building blocks (all sequential building blocks can be used).

The shared-memory version should be able to compress/decompress a single file as well as all files in a directory.

The validation and performance tests must be conducted considering both small (a few hundred KB) and big files (hundreds of MB).

The developed code should be delivered in a tarball (tgz or zip) with a PDF document, a Makefile/Cmake for compiling the source code on the spmcluster machine, and all scripts for running the tests using SLURM. 

The files used for the tests should not be inserted in the project tarball.

The PDF document must describe the parallelization strategy adopted, the performance analysis, the plots of the speedup/scalability/efficiency obtained by the tests, comments, problems faced, etc. 
It should be at most 12 pages long.

---