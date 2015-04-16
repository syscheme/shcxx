cmake_minimum_required(VERSION 2.8)

project(DataPostHouse)

include_directories(${PROJECT_SOURCE_DIR}/../)

add_library(DataPostHouse STATIC DataDialogFactoryI.cpp DataPostHouseEnv.cpp DataPostHouseLinux.cpp
								DataCommunicator_linux.cpp DataCommunicatorSSL_linux.cpp )
