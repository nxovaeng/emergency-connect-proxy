# tests/CMakeLists.txt

add_executable(test_wsnet
    test_wsnet.cpp
    ${CMAKE_SOURCE_DIR}/src/wsnet/WSNet.cpp
    ${CMAKE_SOURCE_DIR}/src/wsnet/emergency_connect_impl.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
)

target_include_directories(test_wsnet PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(UNIX AND NOT APPLE)
    target_link_libraries(test_wsnet PRIVATE pthread)
elseif(APPLE)
    target_link_libraries(test_wsnet PRIVATE pthread)
elseif(WIN32)
    target_compile_definitions(test_wsnet PRIVATE WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
    target_link_libraries(test_wsnet PRIVATE ws2_32 iphlpapi)
endif()

if(MSVC)
    target_compile_definitions(test_wsnet PRIVATE _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
    target_compile_options(test_wsnet PRIVATE /W4 /wd4996)
else()
    target_compile_options(test_wsnet PRIVATE -Wall -Wextra)
endif()

add_test(NAME test_wsnet COMMAND test_wsnet)

