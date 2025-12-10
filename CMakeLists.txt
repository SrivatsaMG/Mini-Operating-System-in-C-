cmake_minimum_required(VERSION 3.16)
project(MiniOS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

include_directories(${INCLUDE_DIR})

set(KERNEL_SOURCES
    ${SRC_DIR}/kernel/kernel.cpp
)

set(SCHEDULER_SOURCES
    ${SRC_DIR}/scheduler/scheduler.cpp
)

set(MM_SOURCES
    ${SRC_DIR}/mm/memory_manager.cpp
)

set(FS_SOURCES
    ${SRC_DIR}/fs/filesystem.cpp
)

set(IPC_SOURCES
    ${SRC_DIR}/ipc/ipc.cpp
)

set(DRIVER_SOURCES
    ${SRC_DIR}/drivers/driver.cpp
)

set(ALL_SOURCES
    ${KERNEL_SOURCES}
    ${SCHEDULER_SOURCES}
    ${MM_SOURCES}
    ${FS_SOURCES}
    ${IPC_SOURCES}
    ${DRIVER_SOURCES}
)

add_library(minios_core STATIC ${ALL_SOURCES})
target_include_directories(minios_core PUBLIC ${INCLUDE_DIR})

add_executable(minios ${SRC_DIR}/main.cpp)
target_link_libraries(minios PRIVATE minios_core pthread)

enable_testing()

add_executable(test_scheduler tests/test_scheduler.cpp)
target_link_libraries(test_scheduler PRIVATE minios_core pthread)
add_test(NAME SchedulerTests COMMAND test_scheduler)

add_executable(test_memory tests/test_memory.cpp)
target_link_libraries(test_memory PRIVATE minios_core pthread)
add_test(NAME MemoryTests COMMAND test_memory)

add_executable(test_filesystem tests/test_filesystem.cpp)
target_link_libraries(test_filesystem PRIVATE minios_core pthread)
add_test(NAME FileSystemTests COMMAND test_filesystem)

add_executable(test_ipc tests/test_ipc.cpp)
target_link_libraries(test_ipc PRIVATE minios_core pthread)
add_test(NAME IPCTests COMMAND test_ipc)

message(STATUS "===========================================")
message(STATUS "MiniOS - Mini Microkernel Operating System")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "===========================================")
