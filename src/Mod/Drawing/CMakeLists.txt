# Drawing workbench main CMakeLists.txt

add_subdirectory(App)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

# Install Python files
INSTALL(FILES
    InitGui.py
    Init.py
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Drawing
)