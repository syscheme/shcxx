cmake_minimum_required(VERSION 2.8)

project(HttpEngine)

include(${PROJECT_SOURCE_DIR}/../../ZQProjs.cmake)

include_directories(${zqcommonfolder} ${zqcommonfolder}/DataPostHouse)

add_library(HttpEngine STATIC ClientRequestGw.cpp HttpEngine.cpp HttpUtil.cpp )

