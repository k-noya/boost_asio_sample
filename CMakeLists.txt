# Common settings
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(boost_asio_sample
  VERSION 0.1.6
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(simple_async_http_client)
