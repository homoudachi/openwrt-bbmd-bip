cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(bbmd VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(BBMD_TLS_BACKEND "openssl" CACHE STRING "TLS backend: openssl or mbedtls")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(LIBUBOX REQUIRED libubox)
pkg_check_modules(LIBUBUS REQUIRED libubus)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)

if(BBMD_TLS_BACKEND STREQUAL "openssl")
  find_package(OpenSSL REQUIRED)
  pkg_check_modules(SSL REQUIRED openssl)
elseif(BBMD_TLS_BACKEND STREQUAL "mbedtls")
  pkg_check_modules(MBEDTLS REQUIRED mbedtls mbedcrypto mbedx509)
endif()

set(BACNET_STACK_BUILD_APPS OFF CACHE BOOL "" FORCE)
set(BACDL_BSC ON CACHE BOOL "" FORCE)
set(BACDL_BIP ON CACHE BOOL "" FORCE)
set(BACDL_BIP6 OFF CACHE BOOL "" FORCE)
set(BACDL_MSTP OFF CACHE BOOL "" FORCE)
set(BACDL_ARCNET OFF CACHE BOOL "" FORCE)
set(BACDL_ETHERNET OFF CACHE BOOL "" FORCE)
set(BACDL_ZIGBEE OFF CACHE BOOL "" FORCE)
set(BAC_ROUTING ON CACHE BOOL "" FORCE)
set(BACNET_PROPERTY_LISTS ON CACHE BOOL "" FORCE)
set(BACNET_SEGMENTATION_ENABLED ON CACHE BOOL "" FORCE)
set(BACNET_BACKUP_RESTORE ON CACHE BOOL "" FORCE)
set(INTRINSIC_REPORTING OFF CACHE BOOL "" FORCE)

add_subdirectory(bacnet)

target_include_directories(bacnet-stack PUBLIC ${LIBWEBSOCKETS_INCLUDE_DIRS})
target_compile_definitions(bacnet-stack PRIVATE
  LWS_MAX_SMP=32
  BSC_CONF_SERVER_HUB_CONNECTIONS_MAX_NUM=50
  SC_NETPORT_CONNECT_TIMEOUT=5
  SC_NETPORT_HEARTBEAT_TIMEOUT=30
  SC_NETPORT_DISCONNECT_TIMEOUT=90
  SC_NETPORT_RECONNECT_TIME=60
  BACNET_FILE_PATH_RESTRICTED=0
)

if(BBMD_TLS_BACKEND STREQUAL "mbedtls")
  target_compile_definitions(bacnet-stack PRIVATE CONFIG_MBEDTLS)
  target_link_libraries(bacnet-stack PUBLIC
    ${MBEDTLS_LIBRARIES})
  target_include_directories(bacnet-stack PUBLIC
    ${MBEDTLS_INCLUDE_DIRS})
endif()

set(BBMD_SOURCES
  bbmd.c
  bbmd_bip.c
  bbmd_bridge.c
  bbmd_config.c
  bbmd_hub.c
  bbmd_node.c
  bbmd_telemetry.c
)

add_executable(bbmd ${BBMD_SOURCES})

target_compile_options(bbmd PRIVATE
  -Os
  -Wall
  -Wextra)

target_include_directories(bbmd PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBUBOX_INCLUDE_DIRS}
  ${LIBUBUS_INCLUDE_DIRS}
  ${LIBWEBSOCKETS_INCLUDE_DIRS})

target_link_libraries(bbmd PRIVATE
  bacnet-stack
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUBOX_LIBRARIES}
  ${LIBUBUS_LIBRARIES}
  Threads::Threads
  m
  rt)

if(BBMD_TLS_BACKEND STREQUAL "openssl")
  target_link_libraries(bbmd PRIVATE OpenSSL::SSL OpenSSL::Crypto)
elseif(BBMD_TLS_BACKEND STREQUAL "mbedtls")
  target_link_libraries(bbmd PRIVATE
    ${MBEDTLS_LIBRARIES})
endif()

install(TARGETS bbmd RUNTIME DESTINATION sbin)
