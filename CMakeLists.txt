cmake_minimum_required(VERSION 3.20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")
option(USE_VENDORED "Use vendored libraries" ON)

project(game-testing)
add_executable(app ${SOURCES})

target_include_directories(app PRIVATE C:/Glad/include)
target_sources(app PRIVATE
    C:/Glad/src/glad.c
)

if(USE_VENDORED)
    set(SDLTTF_VENDORED ON)
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
    add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)
else()
    find_package(SDL3 REQUIRED)
    find_package(SDL3_ttf REQUIRED)
endif()

target_link_libraries(app PRIVATE SDL3_ttf::SDL3_ttf SDL3::SDL3)

target_compile_definitions(app PRIVATE SDL_MAIN_USE_CALLBACKS)
target_link_options(app PRIVATE -mwindows -static)
