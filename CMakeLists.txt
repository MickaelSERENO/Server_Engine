#Minimum cmake version 3.0
cmake_minimum_required(VERSION 3.0)

#Define the project
project(Server CXX)

#Define Outputs (bin, lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set (CMAKE_CXX_STANDARD 14)

#Debug version
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-Wall -Werror")

#Set the sources and the headers
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h)

#PKGConfig configuration
find_package(PkgConfig REQUIRED)

#Configure the executable : sources, compile options (CFLAGS) and link options (LDFLAGS)
add_executable(Server ${SOURCES} ${HEADERS})
target_link_libraries(Server PUBLIC
    pthread)

#Add include directory
target_include_directories(Server PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
