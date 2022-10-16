cmake_minimum_required (VERSION 3.1)
project(lgtbot-kook LANGUAGES CXX C)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

add_subdirectory(lgtbot)

if(MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    message(STATUS "optional: /UTF-8")
endif(MSVC)

add_library(lgtbot_kook SHARED ${CMAKE_CURRENT_SOURCE_DIR}/lgtbot_kook.cc)
set_target_properties(lgtbot_kook PROPERTIES PREFIX "") # prevent prepending 'lib' to library names

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lgtbot /usr/include/python3.6m)

target_link_libraries(lgtbot_kook PRIVATE gflags bot_core python3.6m boost_python3)

add_dependencies(lgtbot_kook bot_core)

add_custom_target(main_py ALL
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy main.py ${CMAKE_CURRENT_BINARY_DIR})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(lgtbot_kook PRIVATE pthread)
endif()
