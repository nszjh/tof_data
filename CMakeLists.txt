cmake_minimum_required (VERSION 2.8.13)
set (targetName  TofData)
PROJECT(${targetName})
add_definitions(-msse2 -std=c++11)

aux_source_directory(. TOF_DATA)
#include_directories()
add_library(tofData  ${TOF_DATA})
