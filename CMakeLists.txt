cmake_minimum_required(VERSION 3.5)

project(http_utils LANGUAGES CXX)

include_directories(${ASIO_DIR})

add_definitions(-DASIO_STANDALONE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
add_definitions(-DASIO_MSVC)
add_definitions(-DBOOST_ASIO_HAS_MOVE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /bigobj")
endif(MSVC)



INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)


# add log support
find_package(spdlog CONFIG REQUIRED)
INCLUDE_DIRECTORIES(${spdlog_include_dirs})

# add enum support
find_package(magic_enum CONFIG REQUIRED)
INCLUDE_DIRECTORIES(${magic_enum_include_dirs})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
link_directories(${OPENSSL_LIBRARY_DIR})


add_definitions (-DBOOST_ASIO_NO_DEPRECATED=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_ARRAY=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_BIND=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_DATE_TIME=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_REGEX=1)

file(GLOB COMMON_SRC  "${PROJECT_SOURCE_DIR}/src/common/*.cpp" "${PROJECT_SOURCE_DIR}/src/common/*.c")
add_library(http_common ${COMMON_SRC})
target_link_libraries(http_common PUBLIC spdlog::spdlog)

file(GLOB HTTP_CLIENT_SRC  "${PROJECT_SOURCE_DIR}/src/http_client/*.cpp")
add_library(http_client ${HTTP_CLIENT_SRC})
target_link_libraries(http_client PUBLIC http_common Threads::Threads)

file(GLOB HTTPS_CLIENT_SRC  "${PROJECT_SOURCE_DIR}/src/https_client/*.cpp")
add_library(https_client ${HTTPS_CLIENT_SRC})
target_link_libraries(https_client PUBLIC http_common Threads::Threads ${OPENSSL_LIBRARIES})

file(GLOB HTTP_SERVER_SRC  "${PROJECT_SOURCE_DIR}/src/http_server/*.cpp")
add_library(http_server ${HTTP_SERVER_SRC})
target_link_libraries(http_server PUBLIC http_common Threads::Threads)


file(GLOB HTTPS_SERVER_SRC  "${PROJECT_SOURCE_DIR}/src/https_server/*.cpp")
add_library(https_server ${HTTPS_SERVER_SRC})
target_link_libraries(https_server PUBLIC http_common Threads::Threads ${OPENSSL_LIBRARIES})


set(TEST_DIR ${PROJECT_SOURCE_DIR}/test/)

add_executable(http_client_test ${TEST_DIR}/http_client_test.cpp)
target_link_libraries(http_client_test http_client)

add_executable(https_client_test ${TEST_DIR}/https_client_test.cpp)
if(MSVC)
target_link_libraries(https_client_test https_client Crypt32.lib)
else()
target_link_libraries(https_client_test https_client)
endif()

add_executable(http_server_test ${TEST_DIR}/http_server_test.cpp)
target_link_libraries(http_server_test http_server)

add_executable(https_server_test ${TEST_DIR}/https_server_test.cpp)
target_link_libraries(https_server_test https_server)



foreach(p LIB BIN INCLUDE)
	set(var CMAKE_INSTALL_${p}DIR)
	if(NOT IS_ABSOLUTE "${${var}}")
		string(TOLOWER ${p} lower_p)
        set(${var} "${CMAKE_INSTALL_PREFIX}/${lower_p}")
	endif()
endforeach()

export(TARGETS http_common http_client http_server https_client https_server
     FILE ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}Targets.cmake)

file(WRITE
    ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
    "include(\${CMAKE_CURRENT_LIST_DIR}/${CMAKE_PROJECT_NAME}Targets.cmake)\n")

install(FILES
        ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME})

install(TARGETS http_common http_client http_server https_client https_server EXPORT ${PROJECT_NAME}Targets
   ARCHIVE  DESTINATION lib
   LIBRARY DESTINATION lib
  )

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/${PROJECT_NAME})

install(EXPORT ${CMAKE_PROJECT_NAME}Targets
		NAMESPACE http_utils::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME})