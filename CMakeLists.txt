cmake_minimum_required(VERSION 3.10)

project(libyapp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(YAPP_BUILD_SHARED "Build YAPP as a shared library." OFF)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE HEADER_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(GLOB_RECURSE SRC_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.tpp)

source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SRC_FILES})

if (YAPP_BUILD_SHARED)
  add_compile_definitions(YAPP_SHARED)
  add_library(libyapp SHARED ${HEADER_FILES} ${SRC_FILES})
else()
  add_compile_definitions(YAPP_STATIC)
  add_library(libyapp STATIC ${HEADER_FILES} ${SRC_FILES})
endif()
  
target_include_directories(libyapp PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
)

enable_testing()
add_executable(testyapp ${PROJECT_SOURCE_DIR}/test/main.cpp ${PROJECT_SOURCE_DIR}/test/framework.hpp)
target_link_libraries(testyapp libyapp)
target_include_directories(testyapp PUBLIC
  "${PROJECT_SOURCE_DIR}/test"
  "${PROJECT_SOURCE_DIR}/include"
)
add_test(NAME testyapp COMMAND testyapp)
