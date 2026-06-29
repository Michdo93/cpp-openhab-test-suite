cmake_minimum_required(VERSION 3.16)
project(OpenHABTestSuite VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── cpp-openhab-rest-client (submodule or find_package) ───────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/cpp-openhab-rest-client/CMakeLists.txt")
  add_subdirectory(extern/cpp-openhab-rest-client)
else()
  find_package(OpenHABRestClient REQUIRED)
endif()

find_package(CURL REQUIRED)

find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
  include(FetchContent)
  FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(json)
endif()

# ── Header-only library ───────────────────────────────────────────────────────
# All testers are header-only; we create an INTERFACE target so consumers
# only need to link openhab_test_suite_headers.
add_library(openhab_test_suite_headers INTERFACE)
target_include_directories(openhab_test_suite_headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(openhab_test_suite_headers INTERFACE
  openhab_rest_client_static
  ${CURL_LIBRARIES}
  nlohmann_json::nlohmann_json)

add_library(OpenHABTestSuite::Headers ALIAS openhab_test_suite_headers)

# ── Test executable ───────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp")
  add_executable(openhab_suite_test test/test.cpp)
  target_link_libraries(openhab_suite_test PRIVATE
    openhab_test_suite_headers
    openhab_rest_client_static
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json)
  if(UNIX)
    target_link_libraries(openhab_suite_test PRIVATE pthread)
  endif()
endif()

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS openhab_test_suite_headers
  EXPORT OpenHABTestSuiteTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

message(STATUS "──────────────────────────────────────────")
message(STATUS " OpenHABTestSuite ${PROJECT_VERSION} (header-only)")
message(STATUS "──────────────────────────────────────────")
