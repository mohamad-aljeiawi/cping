cmake_minimum_required(VERSION 3.22.1)
project(cping_socket)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB IMGUI_SRC
        "include/*.cpp"
        "include/utils/*.cpp"
        "include/debug/*.cpp"
        "include/imgui/*.cpp"
        "include/imgui/backends/*.cpp"
        "include/native/*.cpp"

)

add_library(imgui STATIC ${IMGUI_SRC})

target_include_directories(imgui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/include/debug
        ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/include/native
)


add_library(cping_socket SHARED
        src/main.cpp
)

target_link_libraries(
        cping_socket
        imgui
        GLESv3
        EGL
        log
)
