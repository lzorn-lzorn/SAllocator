set(TARGET_NAME MemDetector)
file(GLOB_RECURSE
    INC
    *.h
)
file(GLOB_RECURSE
    SRC
    *.cpp
)

add_library(${TARGET_NAME} ${INC} ${SRC})