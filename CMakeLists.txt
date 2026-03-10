# 一、CMake版本要求
cmake_minimum_required(VERSION 3.10)

# 二、项目定义（包括项目名称、使用的语言、版本号）
project(MyChatServer
    VERSION 0.0.1
    LANGUAGES CXX
)

# 三、设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# ========== 四、查找依赖包 ==========

# 1. Boost（至少需要 system、thread 组件）
find_package(Boost REQUIRED COMPONENTS system thread)
if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# 2. Threads（用于多线程）
find_package(Threads REQUIRED)

# 3. MySQL（使用 libmysqlclient）
# 首先尝试用 pkg-config 查找（常见于 Linux）
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_MYSQL QUIET mysqlclient)
endif()

# 设置 MySQL 头文件和库的搜索路径
find_path(MYSQL_INCLUDE_DIR
    NAMES mysql.h
    PATHS ${PC_MYSQL_INCLUDE_DIRS}
    /usr/include/mysql
    /usr/local/include/mysql
    /opt/mysql/mysql/include
    PATH_SUFFIXES mysql
)

find_library(MYSQL_LIBRARY
    NAMES mysqlclient mysqlclient_r
    PATHS ${PC_MYSQL_LIBRARY_DIRS}
    /usr/lib/mysql
    /usr/local/lib/mysql
    /opt/mysql/mysql/lib
)

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
    set(MYSQL_FOUND TRUE)
    message(STATUS "MySQL found: includes=${MYSQL_INCLUDE_DIR}, library=${MYSQL_LIBRARY}")
else()
    message(FATAL_ERROR "MySQL not found. Please install libmysqlclient-dev or set MYSQL_INCLUDE_DIR and MYSQL_LIBRARY manually.")
endif()

# ========== 五、包含目录 ==========
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # 让 #include "xxx.hpp" 能直接从 src 下找到
)

# ========== 收集源文件 ==========
# 使用 GLOB 递归获取 src/ 下所有 .cpp 文件（包括子目录）
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# 可选：列出找到的源文件（用于调试）
# message(STATUS "Source files: ${SRC_FILES}")

# ==========六、生成目标文件 ==========
add_executable(chat_server ${SRC_FILES})

# 七、链接库
target_link_libraries(chat_server
    ${Boost_LIBRARIES}
    ${MYSQL_LIBRARY}
    Threads::Threads
)

# 可选：设置运行时输出路径（直接生成在 build 目录下）
set_target_properties(chat_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)