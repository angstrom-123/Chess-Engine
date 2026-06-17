cmake_minimum_required(VERSION 3.12)
project(chess VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python3 COMPONENTS Development REQUIRED)

set(SOURCES
    src/api.cpp 
    src/pythonModule.cpp
)

add_library(chess SHARED ${SOURCES})

target_include_directories(chess 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${Python3_INCLUDE_DIRS}
)

target_link_libraries(chess PRIVATE ${Python3_LIBRARIES})

if(WIN32)
    set_target_properties(chess PROPERTIES SUFFIX ".pyd")
else()
    set_target_properties(chess PROPERTIES SUFFIX ".so")
endif()

message(STATUS "Python module will be built as: $<TARGET_FILE_NAME:chess>")
