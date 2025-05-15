cmake_minimum_required(VERSION 3.22.1)
project(cping_socket)

file(GLOB IMGUI_SRC
        "include/imgui/*.cpp"
        "include/imgui/backends/imgui_impl_opengl3.cpp"
)

add_library(imgui STATIC ${IMGUI_SRC})

target_include_directories(imgui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/backends
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
