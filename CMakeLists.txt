cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
cmake_policy(SET CMP0054 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
enable_testing()
if(UNIX)
  set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/../gtest-build")
endif()

project(superperm VERSION 1.0.0 LANGUAGES C CXX)

include_directories("$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/.")

if(MSVC)
  set(VCPKG_TARGET_TRIPLET "x86-windows" CACHE STRING "VCPKG triplet")

  message(STATUS "MSVC ${VCPKG_TARGET_TRIPLET}")
  
  set(VCPKG_ROOT "C:/vcpkg_2017" CACHE STRING "Path to vcpkg installation")
  
  message(STATUS "vcpkg: ${VCPKG_ROOT}")
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${VCPKG_ROOT}/triplets")
  include(${VCPKG_TARGET_TRIPLET})

  include(${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

  #include_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
  #link_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")
  set(CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
  
 
  # Set Windows 7 as minimum target OS
  add_definitions(-D_WIN32_WINNT=0x0601)
  add_definitions(-DWIN32_LEAN_AND_MEAN)

  # Default to xxxW window functions
  add_definitions(-DUNICODE)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_definitions(-fPIC)
endif()

add_subdirectory(ChaffinMethod)
add_subdirectory(DistributedChaffinMethod)
add_subdirectory(CppDCM)
