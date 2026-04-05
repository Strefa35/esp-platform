# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(ESP_PLATFORM_DIR ${CMAKE_SOURCE_DIR})

set(EXTRA_COMPONENT_DIRS drivers)
list(APPEND EXTRA_COMPONENT_DIRS modules)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-platform)
