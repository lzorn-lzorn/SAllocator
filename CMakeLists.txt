cmake_minimum_required(VERSION 3.20)


project(
    SAllocator
    LANGUAGES CXX C
    DESCRIPTION "SAllocator"
    VERSION 0.1.0
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)


# note: 启用生成 compile_commands.json
set(EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

file(GLOB_RECURSE INC include/*.hpp include/*.h)
file(GLOB_RECURSE SRC src/*.cpp)

add_executable(${PROJECT_NAME} ${SRC})

# set_target_properties(${PROJECT_NAME} PROPERTIES
# LINK_FLAGS "/STACK:4194304 /MANIFEST:NO"
# )
target_link_options(${PROJECT_NAME} PRIVATE
    "/STACK:4194304" # 十进制格式
    "/MANIFEST:NO"
)

# 链接库设置
if (WIN32)
    # 设置平台宏（架构相关）
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32
        _WIN64
        _AMD64_ # ✅ 解决 winnt.h 报错的关键
        WIN32_LEAN_AND_MEAN
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
        NTDDI_VERSION=0x0A00000B
    )

    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE
            /DWIN32 /D_WINDOWS
            /D_USRDLL /D_WINDLL
            /D_UNICODE /DUNICODE
            /MP
        )

        # 可选: 静态 CRT
        if(STATIC_CRT)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
    endif()

    # 设置链接器选项
    target_link_options(${PROJECT_NAME} PRIVATE
        "/STACK:4194304"
        "/MANIFEST:NO"
    )

    # Windows 系统库链接
    target_link_libraries(${PROJECT_NAME}
        kernel32.lib user32.lib gdi32.lib winspool.lib
        shell32.lib ole32.lib oleaut32.lib uuid.lib
        comdlg32.lib advapi32.lib
    )
endif()

# 禁用特定警告（可选）
# target_compile_options(${PROJECT_NAME} PRIVATE
# /W4 /WX
# /wd4100 /wd4201 /wd4189
# )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)