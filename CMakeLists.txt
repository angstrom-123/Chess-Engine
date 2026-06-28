cmake_minimum_required(VERSION 3.12)
project(chess VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(Python3 COMPONENTS Development REQUIRED)

set(SOURCES
    src/bitboard.cpp 
    src/board.cpp 
    src/move.cpp
    src/core.cpp 
    src/searcher.cpp
    src/evaluator.cpp
    src/attackTable.cpp
    src/boardState.cpp
    src/pythonModule.cpp
)

add_library(chess SHARED ${SOURCES})

target_include_directories(chess 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(chess PRIVATE ${Python3_LIBRARIES})

target_compile_options(chess PRIVATE -msse4.2 -mpopcnt)

if(WIN32)
    set_target_properties(chess PROPERTIES SUFFIX ".pyd")
else()
    set_target_properties(chess PROPERTIES SUFFIX ".so")
endif()

message(STATUS "Python module will be built as: $<TARGET_FILE_NAME:chess>")
