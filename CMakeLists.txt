cmake_minimum_required(VERSION 3.0)
project(HDC2010)

# add flags
#set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE -std=c11 -Wall -pedantic -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE -std=c11 -pedantic -lm")



# build
include_directories ("${PROJECT_SOURCE_DIR}/src")
add_executable (hdc2010 src/hdc2010.c)
target_link_libraries(hdc2010)

# install
install(TARGETS hdc2010 DESTINATION bin)
