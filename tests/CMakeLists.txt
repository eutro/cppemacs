message(STATUS "Building tests")

if (${CMAKE_VERSION} VERSION_LESS 3.11)
  message(WARNING "CMake >= 3.11 required to build tests")
  return()
endif()

include(FetchContent)

FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG v3.6.0
  # do not use FindPackage, we need the shared library, which we
  # likely don't have on the system
)

set(BUILD_SHARED_LIBS ON)
if (${CMAKE_VERSION} VERSION_LESS "3.14")
  FetchContent_Populate(Catch2)
  add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
else()
  FetchContent_MakeAvailable(Catch2)
endif()
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

include(CTest)
include(Catch)

set(CPPEMACS_TEST_TARGET ${PROJECT_NAME}_test)
set(CPPEMACS_TEST2_TARGET ${PROJECT_NAME}_test_cxx_11)

add_library(${CPPEMACS_TEST_TARGET} SHARED
  main.cpp
  listeners.cpp
  test_conversions.cpp
  test_user_ptr.cpp
  test_vector.cpp
  test_exceptions.cpp
)

if (NOT CMAKE_VERSION VERSION_LESS "3.16")
  target_precompile_headers(${CPPEMACS_TEST_TARGET}
    PRIVATE common.hpp)
endif()

target_link_libraries(${CPPEMACS_TEST_TARGET} PRIVATE
  cppemacs Catch2::Catch2)

# test with strict C++11 compliance
add_executable(${CPPEMACS_TEST2_TARGET} test_inst_all.cpp)
target_link_libraries(${CPPEMACS_TEST2_TARGET} PRIVATE cppemacs)
set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES CXX_STANDARD 11)
set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES CXX_EXTENSIONS OFF)

add_test(NAME ${CPPEMACS_TEST2_TARGET}
  COMMAND ${CPPEMACS_TEST2_TARGET})

if (NOT Emacs_FOUND)
  message(FATAL_ERROR "Emacs not found! Aborting...")
endif ()

add_test(NAME ${CPPEMACS_TEST_TARGET}
  COMMAND Emacs::emacs -Q --batch --script
  "${CMAKE_CURRENT_SOURCE_DIR}/test.el"
  "$<TARGET_FILE:${CPPEMACS_TEST_TARGET}>")

if (CPPEMACS_Coverage)
  include(CodeCoverage)
  message(STATUS "Testing with code coverage")
  append_coverage_compiler_flags_to_target(${CPPEMACS_TEST_TARGET})
  set(CPPEMACS_SETUP_COVERAGE_ARGS
    EXECUTABLE ctest
    DEPENDENCIES ${CPPEMACS_TEST_TARGET}
    EXCLUDE /usr ${CMAKE_BINARY_DIR})

  if (NOT ((LCOV_PATH AND GENHTML_PATH)
        OR GCOVR_PATH
        OR FASTCOV_PATH))
    message(FATAL_ERROR "No coverage frontend (lcov + genhtml, gcovr, or fastcov)")
  endif ()

  if (LCOV_PATH AND GENHTML_PATH)
    message(STATUS "Coverage: have lcov")
    setup_target_for_coverage_lcov(NAME cppemacs_coverage_lcov
      ${CPPEMACS_SETUP_COVERAGE_ARGS})
  endif ()
  if (GCOVR_PATH)
    message(STATUS "Coverage: have gcovr")
    setup_target_for_coverage_gcovr_xml(NAME cppemacs_coverage_gcovr_xml
      ${CPPEMACS_SETUP_COVERAGE_ARGS})
    setup_target_for_coverage_gcovr_html(NAME cppemacs_coverage_gcovr_html
      ${CPPEMACS_SETUP_COVERAGE_ARGS})
  endif ()
  if (FASTCOV_PATH)
    message(STATUS "Coverage: have fastcov")
    if (GENHTML_PATH)
      set(CPPEMACS_FASTCOV_ARGS)
    else ()
      set(CPPEMACS_FASTCOV_ARGS SKIP_HTML)
    endif ()
    setup_target_for_coverage_fastcov(NAME cppemacs_coverage_fastcov
      ${CPPEMACS_SETUP_COVERAGE_ARGS}
      ${CPPEMACS_FASTCOV_ARGS})
  endif ()
  
endif ()
