cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(smart_band)

idf_build_get_property(partition_table PARTITION_TABLE)
message(STATUS "Partition table: ${partition_table}")
