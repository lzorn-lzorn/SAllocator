set(TARGET_NAME MemoryPool)
file(GLOB_RECURSE
    INC
    *.h
    *.hpp
)
file(GLOB_RECURSE
    SRC
    *.cpp
)

add_library(${TARGET_NAME} ${INC} ${SRC})
target_include_directories(${TARGET_NAME} PUBLIC ${INC})