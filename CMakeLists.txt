cmake_minimum_required(VERSION 3.16)
project(SpaceGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build GLFW from the vendored source tree instead of requiring a system package.
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# ── Sources ───────────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(SpaceGame ${SOURCES})

# ── Include dirs ──────────────────────────────────────────────────────────────
target_include_directories(SpaceGame PRIVATE
    src
    3rdParty/glad/include
    3rdParty/glfw/include
    3rdParty/glm
    3rdParty/stb
)

# ── GLFW ──────────────────────────────────────────────────────────────────────
add_subdirectory(3rdParty/glfw EXCLUDE_FROM_ALL)
target_link_libraries(SpaceGame PRIVATE glfw)

# ── GLAD (compiled as part of the project) ────────────────────────────────────
target_sources(SpaceGame PRIVATE 3rdParty/glad/src/glad.c)

# ── System libs ───────────────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)
target_link_libraries(SpaceGame PRIVATE OpenGL::GL dl pthread)

# ── Copy assets next to the binary so relative paths work ─────────────────────
add_custom_command(TARGET SpaceGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:SpaceGame>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:SpaceGame>/shaders
)