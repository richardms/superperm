cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
cmake_policy(SET CMP0054 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
enable_testing()
if(UNIX)
  set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/../gtest-build")
endif()

project(SPG VERSION 1.0.0 LANGUAGES CXX)

include_directories("$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/.")

if(MSVC)
  message(STATUS "MSVC ${VCPKG_TARGET_TRIPLET}")
  
  set(VCPKG_ROOT "C:/vcpkg_2017" CACHE STRING "Path to vcpkg installation")
  
  message(STATUS "vcpkg: ${VCPKG_ROOT}")
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${VCPKG_ROOT}/triplets")
  include(${VCPKG_TARGET_TRIPLET})

  include(${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

  #include_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
  #link_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")
  set(CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
  
  if("${VCPKG_CRT_LINKAGE}" STREQUAL "static")
    message(STATUS "Setting MT (static) CRT.")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
  else()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
  endif()	
  
  if("${VCPKG_LIBRARY_LINKAGE}" STREQUAL "static")
    message(STATUS "Setting Static linkage.")

	set(BUILD_SHARED_LIBS OFF)
    set(GTEST_CREATE_SHARED_LIBRARY 0)
    set(GTEST_LINKED_AS_SHARED_LIBRARY 0)
  endif()
  
  if("${VCPKG_TARGET_ARCHITECTURE}" STREQUAL "x64")
    message(STATUS "64 bit compilation.")
	if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
		# Compiler is NOT 64 bits
		message(FATAL_ERROR "Non-Win64 Generator in use, remember to use CMake option (e.g.) '-G \"Visual Studio 15 2017 Win64\"'")
	endif()
	set(BUILD_BENPY OFF)
  else()
    message(STATUS "32 bit compilation.")
	if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
		# Compiler is NOT 32 bits
		message(FATAL_ERROR "Non-Win32 Generator in use, remember to use CMake option (e.g.) '-G \"Visual Studio 15 2017\"'")
	endif()
  endif()

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

add_subdirectory(CppDCM)
