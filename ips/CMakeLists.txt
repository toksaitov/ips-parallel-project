#
# CMakeLists.txt
#
# Created by Dmitrii Toksaitov on Fri Aug 30 05:02:16 KGT 2013
#

if(NOT DEFINED PRODUCT_NAME)
    set(PRODUCT_NAME "ips")
endif()

# Directories and Sources

set(PRODUCT_EXECUTABLE ${PRODUCT_NAME})
set(PRODUCT_SOURCES "ips_utils.c"
                    "ips.c")
set_source_files_properties(${PRODUCT_SOURCES} PROPERTIES LANGUAGE CXX)

set(PRODUCT_SHADERS "ips_shader.glsl.vs"
                    "ips_shader.glsl.fs")

# Libraries

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# SDL2

if(NOT SDL2_FOUND)
    find_package(SDL2 REQUIRED)
endif()

# GLEW

if(NOT GLEW_FOUND)
    find_package(GLEW REQUIRED)
endif()

# GLM

if(NOT GLM_FOUND)
    find_package(GLM REQUIRED)
endif()

# OpenGL

find_package(OpenGL REQUIRED)

# zlib

if(NOT ZLIB_FOUND)
    find_package(ZLIB REQUIRED)
endif()

# libpng

if(NOT PNG_FOUND)
    find_package(PNG REQUIRED)
endif()

# Threads

if(NOT WIN32)
    set(CMAKE_THREAD_PREFER_PTHREAD YES)
    find_package(Threads REQUIRED)
endif()

# OpenMP

set(IPS_USE_OPENMP NO CACHE BOOL "Compile with OpenMP support")
if(IPS_USE_OPENMP)
    find_package(OpenMP QUIET)
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
endif()

# Definitions

if(MSVC)
    add_definitions(/D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /wd4068") # ignore the 'unrecognized pragma' warning
endif(MSVC)

if(NOT BUILD_SHARED_LIBS)
    add_definitions(-DIPS_STATIC)
    add_definitions(-DGLEW_STATIC)
    add_definitions(/DPTW32_STATIC_LIB)
endif()

# Includes

include_directories(${PRODUCT_BINARY_DIR} ${SDL2_INCLUDE_DIR}
                                          ${GLEW_INCLUDE_DIRS}
                                          ${GLM_INCLUDE_DIR}
                                          ${OPENGL_INCLUDE_DIR}
                                          ${ZLIB_INCLUDE_DIRS}
                                          ${PNG_INCLUDE_DIRS})

if(WIN32)
    include_directories(${PRODUCT_BINARY_DIR} ${PTHREADS_WIN32_INCLUDE_DIRS})
endif()

# Targets

file(COPY ${PRODUCT_SHADERS} DESTINATION "${CMAKE_BINARY_DIR}/${PRODUCT_NAME}")
if(MSVC)
    file(COPY ${PRODUCT_SHADERS} DESTINATION "${CMAKE_BINARY_DIR}/${PRODUCT_NAME}/Debug")
    file(COPY ${PRODUCT_SHADERS} DESTINATION "${CMAKE_BINARY_DIR}/${PRODUCT_NAME}/Release")
endif()

add_executable(${PRODUCT_EXECUTABLE} ${PRODUCT_SOURCES})
target_link_libraries(${PRODUCT_EXECUTABLE} ${SDL2_LIBRARIES}
                                            ${GLEW_LIBRARIES}
                                            ${OPENGL_LIBRARIES}
                                            ${ZLIB_LIBRARIES}
                                            ${PNG_LIBRARIES})

if(WIN32)
    target_link_libraries(${PRODUCT_EXECUTABLE} ${PTHREADS_WIN32_LIBRARIES})
else()
    target_link_libraries(${PRODUCT_EXECUTABLE} ${CMAKE_THREAD_LIBS_INIT})
endif()
