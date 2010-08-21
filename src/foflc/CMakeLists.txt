project(foflc C)
cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)
set(PROJECT_VERSION "2.34-svn")

FILE(GLOB SOURCE_FILES "*.c")
FILE(GLOB HEADER_FILES "*.h")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

add_executable(foflc ${SOURCES})

install(TARGETS foflc DESTINATION bin)