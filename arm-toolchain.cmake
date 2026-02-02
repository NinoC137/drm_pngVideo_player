#-----------------------------------------------
# Minimal CMake Toolchain File for ARM Cross Compilation
#-----------------------------------------------

# 目标系统
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 交叉编译器
set(CMAKE_C_COMPILER   /bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /bin/arm-linux-gnueabihf-g++)

# 指定交叉 sysroot
set(CMAKE_SYSROOT /opt/sysroot)


# 编译和链接都加上 --sysroot
set(CMAKE_C_FLAGS "--sysroot=${CMAKE_SYSROOT} ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "--sysroot=${CMAKE_SYSROOT} ${CMAKE_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "--sysroot=${CMAKE_SYSROOT} ${CMAKE_EXE_LINKER_FLAGS}")

# find_* 时只在 sysroot 内查找
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# pkgconfig 使用 sysroot
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
set(ENV{PKG_CONFIG_PATH} ${CMAKE_SYSROOT}/usr/lib/arm-linux-gnueabihf/pkgconfig)


