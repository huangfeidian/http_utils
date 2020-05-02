cmake_minimum_required(VERSION 3.5)

project(http_utils LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
endif(MSVC)



INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)

file(GLOB SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# add log support
find_package(spdlog CONFIG REQUIRED)
INCLUDE_DIRECTORIES(${spdlog_include_dirs})

# add enum support
find_package(magic_enum CONFIG REQUIRED)
INCLUDE_DIRECTORIES(${magic_enum_include_dirs})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})
link_directories(${OPENSSL_LIBRARY_DIR})


find_package(Threads REQUIRED)

find_package (
  Boost 1.71 REQUIRED COMPONENTS
	system
	thread
	
	)
include_directories (${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})
add_definitions (-DBOOST_ASIO_NO_DEPRECATED=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_ARRAY=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_BIND=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_DATE_TIME=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_REGEX=1)



add_library(${CMAKE_PROJECT_NAME}  ${SRC_FILES})

target_link_libraries(${CMAKE_PROJECT_NAME}	PUBLIC
	Boost::boost
	Boost::system
	OpenSSL::SSL
	Threads::Threads
	spdlog::spdlog
)

set(TEST_DIR ${PROJECT_SOURCE_DIR}/test/)

add_executable(http_client_test ${TEST_DIR}/http_client_test.cpp)
target_link_libraries(http_client_test ${CMAKE_PROJECT_NAME})

add_executable(https_client_test ${TEST_DIR}/https_client_test.cpp)
target_link_libraries(https_client_test ${CMAKE_PROJECT_NAME} OpenSSL::SSL)

add_executable(http_server_test ${TEST_DIR}/http_server_test.cpp)
target_link_libraries(http_server_test ${CMAKE_PROJECT_NAME})

add_executable(https_server_test ${TEST_DIR}/https_server_test.cpp)
target_link_libraries(https_server_test ${CMAKE_PROJECT_NAME} OpenSSL::SSL)
