find_package(spdlog REQUIRED)
find_package(cpr CONFIG REQUIRED)

add_executable(CppDCM
  CppDCM.cpp
  DCM_static.cpp
  DCM_calc.cpp
  DCM_Client.cpp
)

target_compile_definitions(CppDCM
  PUBLIC FTD2XX_STATIC )

target_include_directories(CppDCM PUBLIC
  .
)

target_link_libraries(CppDCM
PRIVATE
spdlog::spdlog
cpr
legacy_stdio_definitions.lib
${Boost_LIBRARIES}
"-NODEFAULTLIB:LIBCMT"
)
