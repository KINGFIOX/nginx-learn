cmake_minimum_required(VERSION 3.22)

project(nginx)

# ################ 路径设置 ####################

# 搜索目录
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/signal SIGNAL)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/proc PROC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/net NET)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/misc MISC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/app APP)

set(INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/_include")

FILE(GLOB H_FILE "${INCLUDE_PATH}/*.h")

message("current = ${CMAKE_CURRENT_LIST_DIR}")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SIGNAL} ${PROC} ${NET} ${MISC} ${APP} ${H_FILE})

# 头文件搜索路径
target_include_directories(
    ${PROJECT_NAME} PRIVATE
    ${INCLUDE_PATH}
)

# ################## 编译设置 ###################

# 使用c++17标准
target_compile_features(
    ${PROJECT_NAME} PRIVATE cxx_std_17
)

# 默认设置为debug版本
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif(CMAKE_BUILD_TYPE STREQUAL "")

# ################### 输出路径 #####################

# 默认输出路径为./out
set(CMAKE_INSTALL_PREFIX "out")

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin # 输出可执行程序
)
