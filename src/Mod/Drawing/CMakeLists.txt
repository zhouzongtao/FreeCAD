# Drawing workbench main CMakeLists.txt

add_subdirectory(App)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Drawing_Scripts
    Init.py
    DrawingCommands.py
)

if(BUILD_GUI)
    list(APPEND Drawing_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(DrawingScripts ALL
    SOURCES ${Drawing_Scripts}
)

fc_target_copy_resource_flat(DrawingScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Drawing
    ${Drawing_Scripts}
)

INSTALL(
    FILES
        ${Drawing_Scripts}
    DESTINATION
        Mod/Drawing
)