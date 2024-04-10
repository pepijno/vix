cmake_minimum_required(VERSION 3.0.0)
project(vix
    VERSION 0.1
    DESCRIPTION "Vix language"
    LANGUAGES C)
FILE(GLOB MyCSources src/*.c)
FILE(GLOB MyHSources src/*.h)
add_executable(vix ${MyCSources} ${MyHSources})

target_compile_features(vix PRIVATE c_std_23)
target_compile_options(vix PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function)

set(CMAKE_C_FLAGS_DEBUG_INIT "-DDEBUG -g")
set(CMAKE_C_FLAGS_RELEASE_INIT "-O3")
