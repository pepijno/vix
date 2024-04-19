cmake_minimum_required(VERSION 3.0.0)
project(vix
    VERSION 0.1
    DESCRIPTION "Vix language"
    LANGUAGES C)

find_package(Clang REQUIRED)

# Add path to LLVM modules
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${LLVM_CMAKE_DIR}"
  )

# import LLVM CMake functions
include(AddLLVM)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})

FILE(GLOB MyCSources src/*.c)
FILE(GLOB MyHSources src/*.h)
add_llvm_executable(vix ${MyCSources} ${MyHSources})
# set_property(TARGET vix PROPERTY C_STANDARD 23)
target_link_libraries(vix PRIVATE clangTooling)
target_compile_features(vix PRIVATE c_std_23)
target_compile_options(vix PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function)
