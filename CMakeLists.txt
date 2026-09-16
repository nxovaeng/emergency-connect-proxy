cmake_minimum_required(VERSION 3.12)
project(emergency-connect-proxy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选项
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_TESTS "Enable unit tests" OFF)

# 查找依赖
find_package(OpenSSL REQUIRED)

# 源文件
set(SOURCES
    src/main.cpp
    src/emergency_proxy.cpp
    src/openvpn_tunnel.cpp
    src/proxy_server.cpp
    src/system_proxy.cpp
    src/utils/config.cpp
    src/utils/logger.cpp
    src/utils/process.cpp
    src/utils/network.cpp
    src/utils/string_utils.cpp
)

set(HEADERS
    src/emergency_proxy.h
    src/openvpn_tunnel.h
    src/proxy_server.h
    src/system_proxy.h
    src/utils/config.h
    src/utils/logger.h
    src/utils/process.h
    src/utils/network.h
    src/utils/string_utils.h
)

# 创建可执行文件
add_executable(emergency-proxy ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(emergency-proxy PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# 包含目录
target_include_directories(emergency-proxy PRIVATE src)

# 平台特定设置
if(UNIX AND NOT APPLE)
    # Linux
    target_link_libraries(emergency-proxy PRIVATE pthread)
elseif(APPLE)
    # macOS
    target_link_libraries(emergency-proxy PRIVATE pthread)
elseif(WIN32)
    # Windows
    target_link_libraries(emergency-proxy PRIVATE ws2_32 winsock2 iphlpapi)
endif()

# 编译选项
if(MSVC)
    target_compile_options(emergency-proxy PRIVATE /W4)
else()
    target_compile_options(emergency-proxy PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 安装
install(TARGETS emergency-proxy DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/emergency-connect-proxy)
install(FILES README.md LICENSE DESTINATION share/doc/emergency-connect-proxy)

# 测试
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
