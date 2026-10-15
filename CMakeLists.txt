cmake_minimum_required( VERSION 3.14 )
project ( LimitOrderBook )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED )
set( CMAKE_CXX_EXTENSIONS OFF )

enable_testing()

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release )
endif()

add_subdirectory( benchmark )
add_subdirectory( src )
add_subdirectory ( test )