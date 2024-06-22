cmake_minimum_required(VERSION 3.14)

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

set(ALL_LIB 
        gtest_main 
        ${YAMLCPP}
)

set(ALL_SRC_FILE 
        src/log.cc 
        src/config.cc
)

set(ALL_TEST_FILE 
        tests/singleton_test.cc 
        tests/log_test.cc 
        tests/config_test.cc
)

set(LOG_MODULE
        src/log
        src/config.cc
)

set(CONFIG_MODULE
        src/log.cc
        src/config.cc
)

enable_testing()
add_executable(singleton_test tests/singleton_test.cc)
target_link_libraries(singleton_test gtest_main)
add_test(NAME singleton_test COMMAND singleton_test)

add_executable(log_test tests/log_test.cc ${LOG_MODULE})
target_link_libraries(log_test ${ALL_LIB})
add_test(NAME log_test COMMAND log_test)

add_executable(config_test tests/config_test.cc ${CONFIG_MODULE})
target_link_libraries(config_test ${ALL_LIB})
add_test(NAME config_test COMMAND config_test)
