cmake_minimum_required(VERSION 3.20)

project(xl)

set(XL_MINIZ_IMPL "BUILTIN" CACHE STRING "XL miniz implementation")
set_property(CACHE XL_MINIZ_IMPL PROPERTY STRINGS "NONE" "BUILTIN" "EXTERN")

add_library(xl INTERFACE)

target_compile_features(xl INTERFACE cxx_std_20)

target_include_directories(xl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(XL_MINIZ_IMPL STREQUAL "BUILTIN")
    message(STATUS "XL -- using built-in miniz implementation")
    add_subdirectory("ext/miniz")
    target_link_libraries(xl INTERFACE miniz)
elseif(XL_MINIZ_IMPL STREQUAL "EXTERN")
    message(STATUS "XL -- using extern miniz implementation")
    add_subdirectory("ext/miniz-extern")
    target_link_libraries(xl INTERFACE miniz_extern)
endif()


