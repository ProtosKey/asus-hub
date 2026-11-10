cmake_minimum_required(VERSION 3.16)
project(asus-hub-daemon LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED libcjson)
pkg_check_modules(UDEV REQUIRED libudev)

add_executable(asus-hub-daemon
    src/cpu.c
    src/fans.c
    src/gpu.c
    src/main.c
    src/statistics.c
    src/utils.c
)

target_link_libraries(asus-hub-daemon PRIVATE
    ${CJSON_LIBRARIES}
    ${UDEV_LIBRARIES}
)

target_include_directories(asus-hub-daemon PRIVATE
    include
    ${CJSON_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
)

target_compile_options(asus-hub-daemon PRIVATE
    ${CJSON_CFLAGS_OTHER}
    ${UDEV_CFLAGS_OTHER}
)
