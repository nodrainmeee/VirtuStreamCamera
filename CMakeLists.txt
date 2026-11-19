cmake_minimum_required(VERSION 3.24)
project(VirtuStreamCamera VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "VirtuStream Camera cible Windows 10/11 x64.")
endif()

add_subdirectory(app)

include(CTest)
add_subdirectory(tests)
