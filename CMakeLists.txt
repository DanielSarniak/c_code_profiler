cmake_minimum_required(VERSION 3.16)
project(c_code_profiler C)

set(CMAKE_C_STANDARD 11)

# =========================
# INCLUDE DIR
# =========================
include_directories(include)

# =========================
# LIBRARY (LD_PRELOAD)
# =========================
add_library(profiler SHARED
    src/profiler.c
    src/utils.c
)

target_link_libraries(profiler
    dl
    pthread
)

# =========================
# AUTO TEST DETECTION
# =========================
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    tests/*.c
)

set(TEST_NAMES "")

foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)

    list(APPEND TEST_NAMES ${test_name})

    add_executable(${test_name} ${test_file})

    target_include_directories(${test_name} PRIVATE include)

    target_link_libraries(${test_name}
        profiler
    )
endforeach()


# =========================
# TEST LIST TARGET
# =========================
add_custom_target(list_tests
    COMMAND ${CMAKE_COMMAND} -E echo "Available tests:"
    COMMAND ${CMAKE_COMMAND} -E echo "${TEST_NAMES}"
)

# =========================
# CLI RUNNER (./c_code_profiler)
# =========================
add_executable(c_code_profiler
    c_code_profiler.c
)