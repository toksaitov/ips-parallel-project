#
# CMakeLists.txt
#
# Created by Dmitrii Toksaitov on Thu Aug 29 16:37:41 KGT 2013
#

cmake_minimum_required(VERSION 2.8)

# Definitions

set(PRODUCT_NAME "ips")

set(PRODUCT_MAJOR_VERSION 1)
set(PRODUCT_MINOR_VERSION 0)
set(PRODUCT_PATCH_VERSION 0)

set(SOURCE_DIRECTORY ${PRODUCT_NAME})
set(DEPENDENCIES_DIRECTORY "dependencies")

project(${PRODUCT_NAME})

# Dependencies

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Default override")

# SDL2

set(SDL2_NAME "SDL2")
set(SDL2_VERSION_STRING "2.0.1")
set(SDL2_ROOT "${CMAKE_SOURCE_DIR}/${DEPENDENCIES_DIRECTORY}/${SDL2_NAME}-${SDL2_VERSION_STRING}")
if(EXISTS ${SDL2_ROOT})
    set(SDL2_FOUND YES)

    if(BUILD_SHARED_LIBS)
        set(SDL_SHARED ON  CACHE BOOL "Default override")
        set(SDL_STATIC OFF CACHE BOOL "Default override")
    else()
        set(SDL_SHARED OFF CACHE BOOL "Default override")
        set(SDL_STATIC ON  CACHE BOOL "Default override")
    endif()

    set(SDL2_LIBRARY "SDL2")
    if(NOT BUILD_SHARED_LIBS)
        set(SDL2_LIBRARY "${SDL2_LIBRARY}-static")
    endif()
    set(SDL2_LIBRARIES ${SDL2_LIBRARY} "SDL2main")

    set(SDL2_INCLUDE_DIR "${SDL2_ROOT}/include")
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})

    set(DIRECTX     OFF CACHE BOOL "Default override")
    set(RENDER_D3D  OFF CACHE BOOL "Default override")

    add_subdirectory(${SDL2_ROOT})
else()
    unset(SDL2_NAME)
    unset(SDL2_VERSION_STRING)
    unset(SDL2_ROOT)
endif()

# GLEW

set(GLEW_NAME "glew")
set(GLEW_VERSION_STRING "1.10.0")
set(GLEW_ROOT "${CMAKE_SOURCE_DIR}/${DEPENDENCIES_DIRECTORY}/${GLEW_NAME}-${GLEW_VERSION_STRING}")
if(EXISTS ${GLEW_ROOT})
    set(GLEW_FOUND YES)

    if(WIN32)
        set(GLEW_LIBRARY "glew32")
    else()
        set(GLEW_LIBRARY "GLEW")
    endif()
    set(GLEW_LIBRARIES ${GLEW_LIBRARY})

    set(GLEW_INCLUDE_DIR "${GLEW_ROOT}/include")
    set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})

    add_subdirectory(${GLEW_ROOT})
else()
    unset(GLEW_NAME)
    unset(GLEW_VERSION_STRING)
    unset(GLEW_ROOT)
endif()

# GLM

set(GLM_NAME "glm")
set(GLM_VERSION_STRING "0.9.4.6")
set(GLM_ROOT "${CMAKE_SOURCE_DIR}/${DEPENDENCIES_DIRECTORY}/${GLM_NAME}-${GLM_VERSION_STRING}")
if(EXISTS ${GLM_ROOT})
    set(GLM_FOUND YES)

    set(GLM_INCLUDE_DIR ${GLM_ROOT})
    set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})
else()
    unset(GLM_NAME)
    unset(GLM_VERSION_STRING)
    unset(GLM_ROOT)
endif()

# zlib

set(ZLIB_NAME "zlib")
set(ZLIB_VERSION_MAJOR 1)
set(ZLIB_VERSION_MINOR 2)
set(ZLIB_VERSION_PATCH 8)
set(ZLIB_VERSION_STRING "${ZLIB_VERSION_MAJOR}.${ZLIB_VERSION_MINOR}.${ZLIB_VERSION_PATCH}")
set(ZLIB_ROOT "${CMAKE_SOURCE_DIR}/${DEPENDENCIES_DIRECTORY}/${ZLIB_NAME}-${ZLIB_VERSION_STRING}")
if(EXISTS ${ZLIB_ROOT})
    set(ZLIB_FOUND YES)

    set(ZLIB_LIBRARY "zlib")
    if(NOT BUILD_SHARED_LIBS)
        set(ZLIB_LIBRARY "${ZLIB_LIBRARY}static")
    endif()
    set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})

    set(ZLIB_INCLUDE_DIRS ${ZLIB_ROOT} "${CMAKE_BINARY_DIR}/${DEPENDENCIES_DIRECTORY}/${ZLIB_NAME}-${ZLIB_VERSION_STRING}")

    add_subdirectory(${ZLIB_ROOT})
else()
    unset(ZLIB_NAME)
    unset(ZLIB_VERSION_MAJOR)
    unset(ZLIB_VERSION_MINOR)
    unset(ZLIB_VERSION_PATCH)
    unset(ZLIB_VERSION_STRING)
    unset(ZLIB_ROOT)
endif()

# libpng

set(PNG_NAME "libpng")
set(PNG_MAJOR 1)
set(PNG_MINOR 6)
set(PNG_RELEASE 7)
set(PNG_VERSION_STRING "${PNG_MAJOR}.${PNG_MINOR}.${PNG_RELEASE}")
set(PNG_ROOT "${CMAKE_SOURCE_DIR}/${DEPENDENCIES_DIRECTORY}/${PNG_NAME}-${PNG_VERSION_STRING}")
if(EXISTS ${PNG_ROOT})
    set(PNG_FOUND YES)

    if(BUILD_SHARED_LIBS)
        set(PNG_SHARED ON  CACHE BOOL "Default override")
        set(PNG_STATIC OFF CACHE BOOL "Default override")
    else()
        set(PNG_SHARED OFF CACHE BOOL "Default override")
        set(PNG_STATIC ON  CACHE BOOL "Default override")
    endif()

    set(PNG_LIBRARY "png${PNG_MAJOR}${PNG_MINOR}")
    if(NOT BUILD_SHARED_LIBS)
        set(PNG_LIBRARY "${PNG_LIBRARY}_static")
    endif()
    set(PNG_LIBRARIES ${PNG_LIBRARY})

    set(PNG_INCLUDE_DIRS ${PNG_ROOT} "${CMAKE_BINARY_DIR}/${DEPENDENCIES_DIRECTORY}/${PNG_NAME}-${PNG_VERSION_STRING}")

    add_subdirectory(${PNG_ROOT})
else()
    unset(PNG_NAME)
    unset(PNG_MAJOR)
    unset(PNG_MINOR)
    unset(PNG_RELEASE)
    unset(PNG_VERSION_STRING)
    unset(PNG_ROOT)
endif()

# pthread-win32

if(WIN32)
    set(PTHREADS_WIN32_NAME "pthreads-win32")
    set(PTHREADS_WIN32_VERSION_STRING "2.9.1")
    set(PTHREADS_WIN32_ROOT "${CMAKE_SOURCE_DIR}/${DEPENDENCIES_DIRECTORY}/${PTHREADS_WIN32_NAME}-${PTHREADS_WIN32_VERSION_STRING}")
    if(EXISTS ${PTHREADS_WIN32_ROOT})
        set(PTHREADS_WIN32_FOUND YES)

        set(PTHREADS_WIN32_LIBRARY "pthreads-win32")
        if(NOT BUILD_SHARED_LIBS)
            set(PTHREADS_BUILD_STATIC YES)
        endif()
        set(PTHREADS_WIN32_LIBRARIES ${PTHREADS_WIN32_LIBRARY})

        set(PTHREADS_WIN32_INCLUDE_DIR "${PTHREADS_WIN32_ROOT}/pthreads-win32")
        set(PTHREADS_WIN32_INCLUDE_DIRS ${PTHREADS_WIN32_INCLUDE_DIR})

        add_subdirectory(${PTHREADS_WIN32_ROOT})
    else()
        unset(PTHREADS_WIN32_NAME)
        unset(PTHREADS_WIN32_VERSION_STRING)
        unset(PTHREADS_WIN32_ROOT)
    endif()
endif()

# Sources

add_subdirectory(${PRODUCT_NAME})
