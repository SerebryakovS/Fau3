
# (Automatically converted from project Makefile by convert_to_cmake.py.)

# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# include($ENV{ADF_PATH}/CMakeLists.txt)
include(/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/esp-adf/CMakeLists.txt)
# set(EXTRA_COMPONENT_DIRS $ENV{ADF_PATH}/components)
set(EXTRA_COMPONENT_DIRS /home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/esp-adf/components/)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(play_mp3_control)
