cmake_minimum_required(VERSION 3.25)

project(melee_pc_port VERSION 0.0.1 LANGUAGES C CXX)

include(CTest)

option(MELEE_HOST_BUILD_TESTS "Build native host unit tests" ${BUILD_TESTING})
option(MELEE_HOST_WARNINGS_AS_ERRORS "Treat native host warnings as errors" ON)
option(MELEE_HOST_ENABLE_SANITIZERS "Enable ASan and UBSan in native targets" OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(port)

if(MELEE_HOST_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(
        NAME melee-extract-tests
        COMMAND ${Python3_EXECUTABLE} -m unittest discover
                -s ${CMAKE_SOURCE_DIR}/tools/tests
                -p test_*.py
    )
endif()
