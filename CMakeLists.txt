cmake_minimum_required(VERSION 3.10)

# set the project name
project(tinyRaytracer)

#https://stackoverflow.com/questions/42834844/how-to-get-cmake-to-pass-either-std-c14-c1y-or-c17-c1z-based-on-gcc-vers
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRCS
	main.cpp
    )
    
set(HEADERS
    geometry.h
    stb_image_write.h
    )

# add the executable
add_executable(tinyRaytracer ${SRCS} ${HEADERS})