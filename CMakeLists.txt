cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_COMPILER "/usr/bin/clang-14")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-14")

project(Sylar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_definitions(-DDEBUGMODE)
endif()

# Enable AddressSanitizer
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
# set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

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
    dl
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
    src/coroutine.cc
    src/schedule.cc
    src/iomanager.cc
    src/timer.cc
    src/fdcontext.cc
    src/hook.cc
    src/address.cc
    src/socket.cc
    src/bytearray.cc
    src/stream.cc
    src/socketstream.cc
    src/tcp_server.cc
)

set(ALL_TEST_FILE
    tests/singleton_test.cc
    tests/log_test.cc
    tests/config_test.cc
    tests/util_test.cc
    tests/thread_test.cc
    tests/coroutine_test.cc
    tests/schedule_test.cc
    tests/iomanager_test.cc
    tests/address_test.cc
    tests/socket_test.cc
    tests/bytearray_test.cc
    tests/tcpserver_test.cc
)

set(LOG_MODULE src/log.cc)

set(CONFIG_MODULE src/log.cc src/config.cc)

set(UTIL_MODULE src/util.cc)

set(THREAD_MODULE ${LOG_MODULE} ${CONFIG_MODULE} ${UTIL_MODULE} src/thread.cc
                  src/mutex.cc)

set(COROUTINE_MODULE src/coroutine.cc)

set(SCHEDULE_MODULE src/schedule.cc)

enable_testing()

add_executable(singleton_test tests/singleton_test.cc)
target_link_libraries(singleton_test gtest_main)
add_test(NAME singleton_test COMMAND singleton_test)

add_executable(log_test tests/log_test.cc ${LOG_MODULE})
target_link_libraries(log_test ${ALL_LIBS})
add_test(NAME log_test COMMAND log_test)

add_executable(config_test tests/config_test.cc ${CONFIG_MODULE})
target_link_libraries(config_test ${ALL_LIBS})
add_test(NAME config_test COMMAND config_test)

add_executable(thread_test tests/thread_test.cc ${THREAD_MODULE})
target_link_libraries(thread_test ${ALL_LIBS})

add_executable(util_test tests/util_test.cc src/util.cc)
target_link_libraries(util_test ${ALL_LIBS})
add_test(NAME util_test COMMAND util_test)

add_executable(coroutine_test tests/coroutine_test.cc ${ALL_SRC})
target_link_libraries(coroutine_test ${ALL_LIBS})
add_test(NAME coroutine_test COMMAND coroutine_test)

add_executable(schedule_test tests/schedule_test.cc ${ALL_SRC})
target_link_libraries(schedule_test ${ALL_LIBS})
add_test(NAME schedule_test COMMAND schedule_test)

add_executable(iomanager_test tests/iomanager_test.cc ${ALL_SRC})
target_link_libraries(iomanager_test ${ALL_LIBS})
add_test(NAME iomanager_test COMMAND iomanager_test)

add_executable(hook_test tests/hook_test.cc ${ALL_SRC})
target_link_libraries(hook_test ${ALL_LIBS})
add_test(NAME hook_test COMMAND hook_test)

add_executable(address_test tests/address_test.cc ${ALL_SRC})
target_link_libraries(address_test ${ALL_LIBS})
add_test(NAME address_test COMMAND address_test)

add_executable(socket_test tests/socket_test.cc ${ALL_SRC})
target_link_libraries(socket_test ${ALL_LIBS})
add_test(NAME socket_test COMMAND socket_test)

add_executable(bytearray_test tests/bytearray_test.cc ${ALL_SRC})
target_link_libraries(bytearray_test ${ALL_LIBS})
add_test(NAME bytearray_test COMMAND bytearray_test)

add_executable(tcpserver_test tests/tcpserver_test.cc ${ALL_SRC})
target_link_libraries(tcpserver_test ${ALL_LIBS})
add_test(NAME tcpserver_test COMMAND tcpserver_test)