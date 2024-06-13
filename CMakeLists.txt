cmake_minimum_required(VERSION 3.10)

# 项目信息
project(C_RPC)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

# 包含source目录
include_directories(${PROJECT_SOURCE_DIR}/source)
include_directories(${PROJECT_SOURCE_DIR}/source/util)

# 添加source目录下的源文件
file(GLOB SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/source/coroutine/*.cpp
    ${PROJECT_SOURCE_DIR}/source/rpc/*.cpp
    ${PROJECT_SOURCE_DIR}/source/socket/*.cpp
    ${PROJECT_SOURCE_DIR}/source/util/*.cpp
    ${PROJECT_SOURCE_DIR}/source/raft/*.cpp
)

# 添加公共库
add_library(c_rpc_lib STATIC ${SOURCE_FILES})
target_link_libraries(c_rpc_lib PRIVATE pthread)

# 为examples目录下的每个cpp文件生成一个可执行文件
file(GLOB EXAMPLE_FILES ${PROJECT_SOURCE_DIR}/examples/*.cpp)
foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE c_rpc_lib)
    set_target_properties(${EXAMPLE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/examples)
endforeach()

# 为tests目录下的每个cpp文件生成一个可执行文件
file(GLOB TEST_FILES ${PROJECT_SOURCE_DIR}/tests/*.cpp)
foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE  c_rpc_lib)
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/tests)
endforeach()

# 为tests目录下的每个cpp文件生成一个可执行文件
file(GLOB TEST_FILES ${PROJECT_SOURCE_DIR}/tests/raft/*.cpp)
foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE  c_rpc_lib)
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/tests/raft)
endforeach()