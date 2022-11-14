cmake_minimum_required(VERSION 3.0)

project(sMQTTBroker)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
#include_directories(${FTDI_INCLUDE} ${TA1_INCLUDE} ${INTERP11_INCLUDE} ${LIBELF_INCLUDE})

add_subdirectory (tests/topic)

