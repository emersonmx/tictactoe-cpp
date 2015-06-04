cmake_minimum_required(VERSION 2.6)

project(tictactoe)

include(FindPkgConfig)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/gmx/cmake/Modules/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
endif (CMAKE_COMPILER_IS_GNUCC)
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
endif (CMAKE_COMPILER_IS_GNUCXX)

add_subdirectory(gmx/)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/gmx/)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
set(EXTRA_LIBS ${EXTRA_LIBS} ${SDL2_LIBRARY})

find_package(SDL2_image REQUIRED)
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
set(EXTRA_LIBS ${EXTRA_LIBS} ${SDL2_IMAGE_LIBRARY})

add_executable(${PROJECT_NAME}
    tictactoe/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    gmx
    ${EXTRA_LIBS}
)
