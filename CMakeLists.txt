cmake_minimum_required(VERSION 3.19)
project(out)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

option(ENABLE_PYSSA "Enable PySSA" 1)

if(ENABLE_PYSSA)
    find_package(PythonLibs 3.5 REQUIRED)
endif()
find_package(EssaGUI REQUIRED)

# FIXME: This should be imported along with EssaGUI.
find_package(GLEW REQUIRED)

if(ENABLE_PYSSA)
    set(PYSSA_SOURCES 
        src/essagui/PythonREPL.cpp
        src/pyssa/Environment.cpp
        src/pyssa/Object.cpp
    )
else()
    set(PYSSA_SOURCES)
endif()

add_executable(
    out

    src/ConfigLoader.cpp
    src/History.cpp
    src/Object.cpp
    src/ObjectHistory.cpp
    src/SimulationView.cpp
    src/Trail.cpp
    src/World.cpp
    src/main.cpp
    
    src/essagui/EssaCreateObject.cpp
    src/essagui/EssaGUI.cpp
    src/essagui/EssaSettings.cpp
    src/essagui/EssaSplash.cpp
    src/essagui/FocusedObjectGUI.cpp
    src/essagui/SimulationInfo.cpp

    src/glwrapper/Sphere.cpp

    src/math/Plane.cpp
    src/math/Ray.cpp

    ${PYSSA_SOURCES}
)

target_compile_options(out PUBLIC -fdiagnostics-color=always)

function(setup_target targetname)
    target_compile_options(${targetname} PUBLIC -Werror=non-virtual-dtor)

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_compile_options(${targetname} PUBLIC -fsanitize=undefined,address)
        target_link_options(${targetname} PUBLIC -fsanitize=undefined,address)
    else()
        target_compile_options(${targetname} PUBLIC -O3)
    endif()
endfunction()
setup_target(out)

target_link_libraries(out essa-gui LLGL::3D)
if(ENABLE_PYSSA)
    message("Enabling PySSA")
    target_link_libraries(out ${PYTHON_LIBRARIES})
    target_include_directories(out PRIVATE ${PYTHON_INCLUDE_DIRS})
    target_compile_definitions(out PRIVATE ENABLE_PYSSA=1)
endif()
