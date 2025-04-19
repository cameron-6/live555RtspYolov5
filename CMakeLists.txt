cmake_minimum_required(VERSION 3.20)

project(live555Rtsp_yolov5)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")


set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB SOURCES "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.cc")    #在CMakeLists.txt文件目录下获取源文件
#file(GLOB HEAD "*.hpp" "*.h")        #在CMakeLists.txt文件目录下获取头文件

add_executable(${PROJECT_NAME}  ${SOURCES})

# 指定头文件加载路径
target_include_directories(${PROJECT_NAME} PUBLIC
  /home/zwhy/code/src/live555Rtsp_yolov5/inc
)

# target_link_directories(${PROJECT_NAME} PUBLIC
#     ${CMAKE_CURRENT_LIST_DIR}/../../lib
# )

set(TOOLCHAIN_DIR /home/zwhy/rk3568_sdk5.10/buildroot/output/rockchip_atk_dlrk3568/host/bin)
set(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu-c++)
#set (SYSROOT_DIR ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot)
set(SYSROOT #${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot/usr/include 
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/glib-2.0
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/lib/glib-2.0/include
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/rknn
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/rga
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/opencv4
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/gstreamer-1.0
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/BasicUsageEnvironment
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/groupsock
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/liveMedia
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/UsageEnvironment
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/rockchip
            #${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot/usr/include/libsoup-2.4
            ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot/usr/include/json-glib-1.0
            )
set(CMAKE_SYSROOT ${TOOLCHAIN_DIR}/../aarch64-buildroot-linux-gnu/sysroot)

find_package(OpenCV REQUIRED)

include_directories(${SYSROOT} ${OpenCV_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} 
    rockchip_mpp
    BasicUsageEnvironment groupsock liveMedia UsageEnvironment
    rknnrt
    rga
    ${OpenCV_LIBS}
)


