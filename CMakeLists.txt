cmake_minimum_required(VERSION 3.10)
project(spell_checker C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_FLAGS -pthread)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/words
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(spell_checker main.c utility.c utility.h)
target_link_libraries(${PROJECT_NAME} pthread)
