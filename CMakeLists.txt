cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS 
    "src/*.cpp"
)

option(USE_VENDORED "Use vendored libraries" ON)
option(USE_CONSOLE "Use console application" OFF)

project(game-testing)
add_executable(app ${SOURCES})

if(MSVC)
    target_compile_options(app PRIVATE /EHa)
endif()

target_include_directories(app PRIVATE vendored/include)
target_include_directories(app PRIVATE include)

set(SDLTTF_VENDORED ON)

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

find_library(OPENAL_LIBRARY 
    NAMES OpenAL32 
    PATHS ${CMAKE_SOURCE_DIR}/vendored/openal-soft
    NO_DEFAULT_PATH
)

if(NOT OPENAL_LIBRARY)
    message(WARNING "Using vendored OpenAL source build (may have CRT issues)")
    add_subdirectory(vendored/openal-soft EXCLUDE_FROM_ALL)
    set(OPENAL_TARGET OpenAL)
else()
    message(STATUS "Using prebuilt OpenAL from: ${OPENAL_LIBRARY}")
    add_library(OpenAL_prebuilt SHARED IMPORTED)
    set_target_properties(OpenAL_prebuilt PROPERTIES
        IMPORTED_LOCATION ${OPENAL_LIBRARY}
        IMPORTED_IMPLIB ${OPENAL_LIBRARY}
    )
    set(OPENAL_TARGET OpenAL_prebuilt)
endif()

target_include_directories(app PRIVATE vendored/SDL/include)
target_include_directories(app PRIVATE vendored/SDL_ttf/include)
target_include_directories(app PRIVATE vendored/SDL_image/include)
target_include_directories(app PRIVATE vendored/openal-soft/include)

target_link_libraries(app PRIVATE 
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    SDL3_image::SDL3_image
)

target_link_libraries(app PRIVATE ${OPENAL_TARGET})
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:OpenAL>
    $<TARGET_FILE_DIR:app>
    COMMENT "Copying OpenAL32.dll to executable directory"
)

set(ASSETS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/assets)
set(ASSETS_TARGET_DIR $<TARGET_FILE_DIR:app>/assets)

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -D _SOURCE=${ASSETS_SOURCE_DIR}
        -D _TARGET=${ASSETS_TARGET_DIR}
        -P
        ${CMAKE_CURRENT_SOURCE_DIR}/makefiles/check_and_copy_assets.cmake
    COMMENT "Conditionally copying assets to build directory."
)

target_compile_definitions(app PRIVATE SDL_MAIN_USE_CALLBACKS)
target_link_libraries(app PRIVATE 
    pthread
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Executable will be in: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Building 64-bit executable")
else()
    message(STATUS "Building 32-bit executable")
endif()