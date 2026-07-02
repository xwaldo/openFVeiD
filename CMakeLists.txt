cmake_minimum_required(VERSION 3.15)
project(FVDPlusPlus VERSION 0.9.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Dependencies ---
include(FetchContent)

# GLFW (Windowing)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.3.8)
FetchContent_MakeAvailable(glfw)

# Dear ImGui (UI)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)
FetchContent_MakeAvailable(imgui)

# ImGui manual library setup
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

# ImPlot (Graphs)
FetchContent_Declare(implot GIT_REPOSITORY https://github.com/epezent/implot.git GIT_TAG master)
FetchContent_MakeAvailable(implot)

# --- PATCH IMPLOT FOR 4 Y-AXES ---
file(READ "${implot_SOURCE_DIR}/implot.h" IMPLOT_HEADER)
if(NOT IMPLOT_HEADER MATCHES "ImAxis_Y4")
    string(REPLACE "ImAxis_Y3,     // disabled by default" "ImAxis_Y3,     // disabled by default\n    ImAxis_Y4,     // patched in by FVD++" IMPLOT_HEADER "${IMPLOT_HEADER}")
    file(WRITE "${implot_SOURCE_DIR}/implot.h" "${IMPLOT_HEADER}")
endif()

add_library(implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp
)
target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PRIVATE imgui) # Critical: ImPlot needs ImGui

# --- Dependencies ---
# GLM and GLEW
if(WIN32)
    find_package(glm REQUIRED)
    find_package(GLEW REQUIRED)
    set(GLM_INCLUDE_DIR ${glm_INCLUDE_DIRS})
    set(GLEW_INCLUDE_DIR ${GLEW_INCLUDE_DIRS})
    set(GLEW_LIBRARY GLEW::glew_s)
else()
    # Try finding system packages first (CI or globally installed)
    find_package(glm QUIET)
    find_package(GLEW QUIET)

    if(glm_FOUND AND GLEW_FOUND)
        set(GLM_INCLUDE_DIR ${glm_INCLUDE_DIRS})
        set(GLEW_INCLUDE_DIR ${GLEW_INCLUDE_DIRS})
        set(GLEW_LIBRARY GLEW::GLEW)
    else()
        # Fallback to local dependencies directory for local dev
        set(DEPS_ROOT "/path/to/dependencies")
        set(GLM_INCLUDE_DIR "${DEPS_ROOT}/glm-1.0.3")
        set(GLEW_ROOT "${DEPS_ROOT}/glew-2.3.1")
        set(GLEW_INCLUDE_DIR "${GLEW_ROOT}/include")
        set(GLEW_LIBRARY "${GLEW_ROOT}/lib/libGLEW.so")
    endif()
endif()

# --- Embedded Assets ---
add_executable(bin2c tools/bin2c.cpp)
file(GLOB ASSETS_SHADERS RELATIVE ${CMAKE_SOURCE_DIR} "shaders/*")
file(GLOB ASSETS_FONTS RELATIVE ${CMAKE_SOURCE_DIR} "fonts/*")
set(ASSETS ${ASSETS_SHADERS} ${ASSETS_FONTS})

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/embedded_assets.cpp
    COMMAND bin2c ${CMAKE_CURRENT_BINARY_DIR}/embedded_assets.cpp ${ASSETS}
    DEPENDS bin2c ${ASSETS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating embedded assets"
)

# --- Sources ---
file(GLOB_RECURSE CORE_SOURCES "core/*.cpp")
set(RENDERER_SOURCES
    renderer/glframebuffer.cpp
    renderer/gltexture.cpp
    renderer/shaderprogram.cpp
    renderer/trackmesh.cpp
    renderer/viewport.cpp
)

if(WIN32)
    set(RES_FILE resources.rc)
endif()

add_executable(fvd
    main.cpp
    ${CORE_SOURCES}
    ${RENDERER_SOURCES}
    ui/graphview.cpp
    ui/resultinggraphprocessor.cpp
    ui/measurementgraphprocessor.cpp
    ui/leftpanel.cpp
    ui/transitionview.cpp
    ${RES_FILE}
    ${CMAKE_CURRENT_BINARY_DIR}/embedded_assets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
target_compile_definitions(fvd PRIVATE GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
target_include_directories(fvd PRIVATE
    .
    core/
    renderer/
    ext/
    ${GLM_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
)

# OS Specific libs
if(UNIX AND NOT APPLE)
    target_link_libraries(fvd PRIVATE
        glfw
        ${GLEW_LIBRARY}
        imgui
        implot
        GL
        X11
        pthread
        dl
    )
    target_link_options(fvd PRIVATE -rdynamic)
elseif(APPLE)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    target_link_libraries(fvd PRIVATE
        glfw
        ${GLEW_LIBRARY}
        imgui
        implot
        ${OPENGL_LIBRARY}
    )
elseif(WIN32)
    target_link_libraries(fvd PRIVATE
        glfw
        ${GLEW_LIBRARY}
        imgui
        implot
        opengl32
        dbghelp
        winmm
    )
    target_compile_definitions(fvd PRIVATE GLEW_STATIC NOMINMAX)
endif()


