cmake_minimum_required(VERSION 3.10)

project(SignalSlot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

file(GLOB SRC_FILES
    "test.cpp"
)

# Add include directories
include_directories(
    .
    ~/code/boost_1_82_0
)

# Library directories
link_directories(
    ~/code/boost_1_82_0/stage/lib
)

# Libraries to link
set(LIBS
    boost_system 
    boost_locale 
    boost_filesystem
    pthread
    dl
    z
)

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
