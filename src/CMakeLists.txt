add_library( obe
    bookSide.cpp
    matchingEngine.cpp
    priceLevel.cpp
)

target_include_directories( obe
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_options( obe
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)