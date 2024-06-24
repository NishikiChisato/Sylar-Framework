cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_COMPILER "/usr/bin/clang-14")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-14")

project(Sylar)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_definitions(-DDEBUGMODE)
endif()

# gtest framework

include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Now simply link against gtest or gtest_main as needed. Eg
# add_executable(example unit_test.cc) target_link_libraries(example gtest_main)
# add_test(NAME example_test COMMAND example)

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

set(ALL_LIBS 
        gtest_main 
        ${YAMLCPP}
        ${PTHREAD}
)

set(ALL_SRC 
        src/log.cc 
        src/config.cc
        src/thread.cc
        src/mutex.cc
        src/util.cc
)

set(ALL_TEST_FILE 
        tests/singleton_test.cc 
        tests/log_test.cc 
        tests/config_test.cc
)

set(LOG_MODULE
        src/log.cc
)

set(CONFIG_MODULE
        src/log.cc
        src/config.cc
)

set(THREAD_MODULE 
        src/thread.cc
        src/mutex.cc
)

set(UTIL 
        src/util.cc
)

enable_testing()

add_executable(singleton_test tests/singleton_test.cc)
target_link_libraries(singleton_test gtest_main)
add_test(NAME singleton_test COMMAND singleton_test)

add_executable(log_test tests/log_test.cc ${LOG_MODULE})
target_link_libraries(log_test ${ALL_LIBS})
add_test(NAME log_test COMMAND log_test)

add_executable(config_test tests/config_test.cc ${ALL_SRC})
target_link_libraries(config_test ${ALL_LIBS})
add_test(NAME config_test COMMAND config_test)

add_executable(thread_test tests/thread_test.cc ${ALL_SRC})
target_link_libraries(thread_test ${ALL_LIBS})

add_executable(util_test tests/util_test.cc src/util.cc)
target_link_libraries(util_test ${ALL_LIBS})
add_test(NAME util_test COMMAND util_test)