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

file(GLOB_RECURSE INC *.hpp *.h)
file(GLOB_RECURSE SRC src/*.cpp *.cpp)

add_executable(${PROJECT_NAME} ${SRC})

# 链接库设置
target_link_libraries(${PROJECT_NAME}
    kernel32.lib user32.lib gdi32.lib winspool.lib
    shell32.lib ole32.lib oleaut32.lib uuid.lib
    comdlg32.lib advapi32.lib
)

# 禁用特定警告（可选）
# target_compile_options(${PROJECT_NAME} PRIVATE
# /W4 /WX
# /wd4100 /wd4201 /wd4189
# )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)