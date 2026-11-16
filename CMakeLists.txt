cmake_minimum_required(VERSION 3.16)
project(OpenHABTestSuiteCpp VERSION 2.0.0 LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(OPENHAB_TESTSUITE_TOP_LEVEL ON)
else()
  set(OPENHAB_TESTSUITE_TOP_LEVEL OFF)
endif()
option(OPENHAB_TESTSUITE_BUILD_TESTS "Build the tests of the test suite" ${OPENHAB_TESTSUITE_TOP_LEVEL})
option(OPENHAB_TESTSUITE_INSTALL "Generate install rules" ${OPENHAB_TESTSUITE_TOP_LEVEL})
set(OPENHAB_REST_CLIENT_DIR "" CACHE PATH "Source directory of cpp-openhab-rest-client (optional)")

# ── cpp-openhab-rest-client: target already there, local sources, installed or download ──
if(NOT TARGET openhab::rest_client_cpp)
  if(OPENHAB_REST_CLIENT_DIR)
    add_subdirectory(${OPENHAB_REST_CLIENT_DIR} cpp-openhab-rest-client)
  else()
    find_package(OpenHABRestClientCpp 2 CONFIG QUIET)
    if(NOT OpenHABRestClientCpp_FOUND)
      include(FetchContent)
      # an installed test suite needs the client installed alongside
      set(OPENHAB_INSTALL ${OPENHAB_TESTSUITE_INSTALL} CACHE BOOL "" FORCE)
      FetchContent_Declare(cpp_openhab_rest_client
        GIT_REPOSITORY https://github.com/Michdo93/cpp-openhab-rest-client.git
        GIT_TAG 2.0.0)
      FetchContent_MakeAvailable(cpp_openhab_rest_client)
    endif()
  endif()
endif()

# ── Library ───────────────────────────────────────────────────────────────────
add_library(openhab_test_suite_cpp STATIC
  src/support.cpp
  src/ItemTester.cpp
  src/OtherTesters.cpp)
set_target_properties(openhab_test_suite_cpp PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(openhab_test_suite_cpp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(openhab_test_suite_cpp PUBLIC openhab::rest_client_cpp)
target_compile_features(openhab_test_suite_cpp PUBLIC cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(openhab_test_suite_cpp PRIVATE -Wall -Wextra)
endif()
# the same name in the build tree and after find_package(OpenHABTestSuiteCpp)
set_target_properties(openhab_test_suite_cpp PROPERTIES EXPORT_NAME test_suite_cpp)
add_library(openhab::test_suite_cpp ALIAS openhab_test_suite_cpp)

# ── Tests ─────────────────────────────────────────────────────────────────────
if(OPENHAB_TESTSUITE_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
  enable_testing()
  add_executable(openhab_testsuite_unit test/unit_test.cpp)
  target_link_libraries(openhab_testsuite_unit PRIVATE openhab_test_suite_cpp)
  add_test(NAME unit COMMAND openhab_testsuite_unit)
  add_executable(openhab_testsuite_test test/testsuite_test.cpp)
  target_link_libraries(openhab_testsuite_test PRIVATE openhab_test_suite_cpp)
  add_test(NAME testsuite COMMAND openhab_testsuite_test)
  set_tests_properties(testsuite PROPERTIES SKIP_RETURN_CODE 77)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
if(OPENHAB_TESTSUITE_INSTALL)
  include(GNUInstallDirs)
  install(TARGETS openhab_test_suite_cpp EXPORT OpenHABTestSuiteCppTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(EXPORT OpenHABTestSuiteCppTargets NAMESPACE openhab::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenHABTestSuiteCpp)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/OpenHABTestSuiteCppConfig.cmake
  "include(CMakeFindDependencyMacro)
  find_dependency(OpenHABRestClientCpp 2)
  include(\${CMAKE_CURRENT_LIST_DIR}/OpenHABTestSuiteCppTargets.cmake)
  ")
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenHABTestSuiteCppConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenHABTestSuiteCpp)
endif()

message(STATUS "cpp-openhab-test-suite ${PROJECT_VERSION}")
