cmake_minimum_required(VERSION 2.8)
enable_testing()

enable_language (C)
enable_language (CXX)
enable_language (Fortran OPTIONAL)

add_subdirectory(src)
add_subdirectory(share)
