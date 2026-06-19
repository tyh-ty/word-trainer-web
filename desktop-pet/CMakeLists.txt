cmake_minimum_required(VERSION 3.20)
project(DesktopPet VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost.JSON. Set BOOST_ROOT as an environment variable when Boost is not in
# the default CMake search paths. On the original development machine this also
# works with Scoop's mingw-nuwen package.
if(NOT DEFINED BOOST_ROOT)
    if(DEFINED ENV{BOOST_ROOT})
        set(BOOST_ROOT "$ENV{BOOST_ROOT}")
    elseif(DEFINED ENV{USERPROFILE} AND EXISTS "$ENV{USERPROFILE}/scoop/apps/mingw-nuwen/20.0")
        set(BOOST_ROOT "$ENV{USERPROFILE}/scoop/apps/mingw-nuwen/20.0")
    endif()
endif()
find_package(Boost REQUIRED COMPONENTS json)

set(WEBVIEW2_VERSION "1.0.4022.49")
set(WEBVIEW2_DIR "${CMAKE_SOURCE_DIR}/third_party/webview2/${WEBVIEW2_VERSION}")
set(WEBVIEW2_NUPKG "${CMAKE_SOURCE_DIR}/third_party/webview2/microsoft.web.webview2.${WEBVIEW2_VERSION}.nupkg")
set(WEBVIEW2_URL "https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/${WEBVIEW2_VERSION}/microsoft.web.webview2.${WEBVIEW2_VERSION}.nupkg")

if(NOT EXISTS "${WEBVIEW2_DIR}/build/native/include/WebView2.h")
    message(STATUS "Downloading WebView2 SDK ${WEBVIEW2_VERSION}")
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/webview2")
    file(DOWNLOAD "${WEBVIEW2_URL}" "${WEBVIEW2_NUPKG}" SHOW_PROGRESS)
    file(MAKE_DIRECTORY "${WEBVIEW2_DIR}")
    file(ARCHIVE_EXTRACT INPUT "${WEBVIEW2_NUPKG}" DESTINATION "${WEBVIEW2_DIR}")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WEBVIEW2_RUNTIME_DIR "${WEBVIEW2_DIR}/runtimes/win-x64/native")
else()
    set(WEBVIEW2_RUNTIME_DIR "${WEBVIEW2_DIR}/runtimes/win-x86/native")
endif()

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} src/resource.rc)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    "${WEBVIEW2_DIR}/build/native/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Boost_LIBRARIES}
    d2d1
    dwrite
    dwmapi
    winhttp
    windowscodecs
    winmm
    ws2_32
    sapi
    psapi
    gdi32
    ole32
    oleaut32
    uuid
    shlwapi
    comctl32
    shell32
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/res"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/../web"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/web"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/config.json"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WEBVIEW2_RUNTIME_DIR}/WebView2Loader.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/WebView2Loader.dll"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
