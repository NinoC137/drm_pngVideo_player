cmake_minimum_required(VERSION 3.5)

project(boot_img VERSION 0.1 LANGUAGES C CXX)

# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 -DNDEBUG")

# 交叉编译工具链会在 toolchain 文件中指定
# set(CMAKE_TOOLCHAIN_FILE path/to/armtoolchain.cmake)

# 源文件
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp" "main.cpp")
file(GLOB_RECURSE PROJECT_HEADERS "include/*.h")

add_executable(boot_img ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# include dirs
include_directories(
    include
    ${CMAKE_SYSROOT}/usr/include
)

# 链接 DRM 和 PNG 库
target_link_libraries(boot_img
    PRIVATE
    drm
    png
    m
)

