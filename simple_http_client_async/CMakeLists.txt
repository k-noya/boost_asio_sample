find_package(Boost 1.75.0 REQUIRED)

message("-- Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")

add_executable(simple_http_client_async "")

target_sources(simple_http_client_async
  PRIVATE
    http.cpp
    http_client.cpp
    main.cpp
)

message("-- PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

target_include_directories(simple_http_client_async
  PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

target_compile_options(simple_http_client_async
  PRIVATE
    "-Wall"
)

target_link_libraries(simple_http_client_async
  PRIVATE
    pthread
)
