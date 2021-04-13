project(pngReorderer)

cmake_minimum_required(VERSION 2.8)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

macro( config_project PROJNAME LIBNAME )
    include_directories( ${${LIBNAME}_INCLUDE_DIRS} )
    target_link_libraries( ${PROJNAME} ${${LIBNAME}_LIBRARIES} ${${LIBNAME}_LIBS} )
endmacro()

find_package(PkgConfig REQUIRED)
macro( config_project_by_pkgconfig PROJNAME LIBNAME )
    pkg_check_modules(PKG_${LIBNAME} ${LIBNAME})
    link_directories( ${PKG_${LIBNAME}_LIBRARY_DIRS} )
    include_directories( ${PKG_${LIBNAME}_INCLUDE_DIRS} )
    target_link_libraries(${PROJNAME} ${PKG_${LIBNAME}_LIBRARIES} )
endmacro()

### g++ options
add_definitions(-std=c++17)
add_definitions(-Wall)
add_definitions(-ffast-math)
add_definitions(-O3)
add_definitions(-DNDEBUG)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lm")

SET(projectSRC main.cpp
                      CCRC32.h CCRC32.cpp
                      CChunk.h CChunk.cpp
                      CPNG.h CPNG.cpp
             )
add_executable(${PROJECT_NAME} ${projectSRC})
target_link_libraries(${PROJECT_NAME} stdc++fs)
