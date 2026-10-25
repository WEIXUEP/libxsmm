cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(libxsmm C CXX)

if(NOT WIN32)
  message(WARNING
    "CMake is the not preferred tool to build libxsmm for the time being. It's only for some target usages and \"make\" is the tool of choice."
)
endif()

option(XSMM_STATIC      "static build" ON)
option(XSMM_TRACE       "enable runtime tracing" OFF)
if(WIN32 AND NOT XSMM_STATIC)
  message(FATAL_ERROR "The Windows CMake build currently supports XSMM_STATIC=ON")
endif()

# display config
message(STATUS "******** Build Summary ********")
message(STATUS "General:")
message(STATUS "  CMake version         : ${CMAKE_VERSION}")
message(STATUS "  CMake command         : ${CMAKE_COMMAND}")
message(STATUS "  System                : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Options:")
message(STATUS "  XSMM_STATIC           : ${XSMM_STATIC}")
message(STATUS "  XSMM_TRACE            : ${XSMM_TRACE}")

# Platform
set(LINUX False)
set(MACOS False)
set(WINDOWS False)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(LINUX True)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(WINDOWS True)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(MACOS True)
endif()

# Setup current directory as project root directory.
set(XSMM_ROOT_DIR ${PROJECT_SOURCE_DIR})

set(CMAKE_INSTALL_MESSAGE NEVER)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB XSMM_SRCS LIST_DIRECTORIES false CONFIGURE_DEPENDS ${XSMM_ROOT_DIR}/include/libxsmm/*.c)
list(REMOVE_ITEM XSMM_SRCS ${XSMM_ROOT_DIR}/include/libxsmm/libxsmm_generator_gemm_driver.c)



if(NOT XSMM_SRCS)
  file(GLOB XSMM_SRCS LIST_DIRECTORIES false CONFIGURE_DEPENDS ${XSMM_ROOT_DIR}/src/*.c)
  list(REMOVE_ITEM XSMM_SRCS ${XSMM_ROOT_DIR}/src/libxsmm_generator_gemm_driver.c)
endif()

list(FILTER XSMM_SRCS EXCLUDE REGEX "/libxsmm_(generator_gemm_driver|binaryexport_generator)\\.c$")
set(XSMM_INCLUDE_DIRS ${XSMM_ROOT_DIR}/include)

if(XSMM_STATIC)
  add_library(xsmm STATIC ${XSMM_SRCS})
else()
  add_library(xsmm SHARED ${XSMM_SRCS})
endif()

add_library(LIBXSMM::xsmm ALIAS xsmm)
target_include_directories(xsmm PUBLIC
  $<BUILD_INTERFACE:${XSMM_INCLUDE_DIRS}>
  $<INSTALL_INTERFACE:include>
)

# https://github.com/libxsmm/libxsmm/tree/main#link-instructions
# Please pass BLAS config by "-D__BLAS=0"
# target_compile_definitions(xsmm PRIVATE __BLAS=0)

target_compile_definitions(xsmm PRIVATE LIBXSMM_BUILD PUBLIC LIBXSMM_DEFAULT_CONFIG)
if(XSMM_TRACE)
  target_compile_definitions(xsmm PRIVATE __TRACE=1)
endif()
if(MSVC)
  target_compile_definitions(xsmm PRIVATE _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES
    WIN32_LEAN_AND_MEAN NOMINMAX)
  target_compile_options(xsmm PRIVATE /bigobj /W3)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(xsmm PUBLIC Threads::Threads)
target_link_libraries(xsmm PUBLIC ${CMAKE_DL_LIBS})

include(CheckLibraryExists)
check_library_exists(m sqrt "" XSMM_LIBM)
if(XSMM_LIBM)
  target_link_libraries(xsmm PUBLIC m)
endif()
check_library_exists(rt sched_yield "" XSMM_LIBRT)
if(XSMM_LIBRT)
  target_link_libraries(xsmm PUBLIC rt)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(TARGETS xsmm EXPORT LIBXSMMTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h" PATTERN "libxsmm_source.h" EXCLUDE)
install(EXPORT LIBXSMMTargets NAMESPACE LIBXSMM::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LIBXSMM)
configure_package_config_file(cmake/LIBXSMMConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/LIBXSMMConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LIBXSMM)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LIBXSMMConfig.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LIBXSMM)

include(CTest)
if(BUILD_TESTING)
  add_executable(xsmm-jit-test tests/cmake-jit.c)
  target_link_libraries(xsmm-jit-test PRIVATE LIBXSMM::xsmm)
  add_test(NAME xsmm-jit COMMAND xsmm-jit-test)
  add_executable(xsmm-jit-stack-test tests/cmake-jit-stack.c)
  target_link_libraries(xsmm-jit-stack-test PRIVATE LIBXSMM::xsmm)
  add_test(NAME xsmm-jit-stack COMMAND xsmm-jit-stack-test)
  add_executable(xsmm-registry-test tests/registry.c)
  target_link_libraries(xsmm-registry-test PRIVATE LIBXSMM::xsmm)
  add_test(NAME xsmm-registry COMMAND xsmm-registry-test)
endif()
