# cmake -S . -B build
# make -C build VERBOSE=1

cmake_minimum_required(VERSION 3.10)

project(web_server VERSION 1.0)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAUTODIFF_DEBUG -DDEBUG_CURL_JSON")

set(PROJ web_server)
set(INC_DIR ${CMAKE_SOURCE_DIR}/inc)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin)
file(MAKE_DIRECTORY ${BIN_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SRC_FILES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c")

find_package(OpenSSL REQUIRED)

add_executable(${PROJ} ${SRC_FILES})

target_include_directories(${PROJ} PRIVATE ${INC_DIR})
target_link_libraries(${PROJ} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

set_target_properties(${PROJ} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})