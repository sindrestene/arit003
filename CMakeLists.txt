cmake_minimum_required(VERSION 3.10)
project(arit003 C)

set(CMAKE_C_STANDARD 99)

enable_language(C)

add_executable(arit003
        arit003_tests.c
        arit003.c
        arit003_struct.c
        arit003_buckets.c
        )

target_link_libraries(arit003 m)
