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

enable_testing()
add_executable(singleton_test tests/singleton_test.cc)
target_link_libraries(singleton_test gtest_main)
add_test(NAME singleton_test COMMAND singleton_test)

add_executable(log_test tests/log_test.cc)
target_link_libraries(log_test gtest_main)
add_test(NAME log_test COMMAND log_test)
