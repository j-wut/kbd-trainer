# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Project name
project(KBDTrainer VERSION 1.0 LANGUAGES C)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Where to look for prebuilt SDL3 libraries on Windows
if(WIN32)
    set(SDL3_DIR "C:\\SDL\\SDL3" CACHE PATH "")
    set(SDL3_image_DIR "C:\\SDL\\SDL3_image" CACHE PATH "")
    set(SDL3_ttf_DIR "C:\\SDL\\SDL3_ttf" CACHE PATH "")
endif()

# Use SDL3, SDL3_image and SDL3_ttf from the system when they are installed.
# Otherwise (or with -DKBD_VENDOR_SDL=ON) download and build them statically,
# which works on distros that don't package SDL3 yet.
option(KBD_VENDOR_SDL "Always download and build SDL3 instead of using the system copy" OFF)

if(NOT KBD_VENDOR_SDL)
    find_package(SDL3 CONFIG QUIET)
    find_package(SDL3_image CONFIG QUIET)
    find_package(SDL3_ttf CONFIG QUIET)
endif()

if(TARGET SDL3::SDL3 AND TARGET SDL3_image::SDL3_image AND TARGET SDL3_ttf::SDL3_ttf)
    message(STATUS "Using system SDL3")
    set(KBD_USING_SYSTEM_SDL ON)
else()
    message(STATUS "Downloading and building SDL3 (static)")
    set(KBD_USING_SYSTEM_SDL OFF)
    include(FetchContent)

    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

    set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_JPG OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)

    set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
    set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLTTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
    set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.30
        GIT_SHALLOW TRUE)
    FetchContent_Declare(SDL3_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG release-3.2.6
        GIT_SHALLOW TRUE
        GIT_SUBMODULES "external/libpng;external/zlib")
    FetchContent_Declare(SDL3_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG release-3.2.2
        GIT_SHALLOW TRUE
        GIT_SUBMODULES "external/freetype")
    FetchContent_MakeAvailable(SDL3 SDL3_image SDL3_ttf)
endif()

set(HEADER_FILES
    include/input.h
    include/render.h
    include/game.h
)

# Manually specify source files
set(SOURCE_FILES
    src/main.c
    src/input.c
    src/render.c
    src/game.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf SDL3::SDL3)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _AMD64_)
endif()

# Build the exe in the base project folder
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin"
)

# Copy assets next to the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Copying assets to output directory"
)

# On Windows with system SDL3, copy the DLLs next to the exe
if(WIN32 AND KBD_USING_SYSTEM_SDL)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3>"
            "$<TARGET_FILE:SDL3_image::SDL3_image>"
            "$<TARGET_FILE:SDL3_ttf::SDL3_ttf>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying SDL3 DLLs to output directory"
    )
endif()
