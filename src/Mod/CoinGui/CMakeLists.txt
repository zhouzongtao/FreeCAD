# SPDX-License-Identifier: LGPL-2.1-or-later

# CoinGui - Coin3D Rendering Backend Module

# Source files
set(CoinGui_SRCS
    PreCompiled.h
    PreCompiled.cpp
    AppCoinGui.cpp
    CoinBackendFactory.h
    CoinBackendFactory.cpp
    CoinViewer.h
    CoinViewer.cpp
)

# Create shared library
add_library(CoinGui SHARED ${CoinGui_SRCS})

# Include directories - MUST be set before precompiled headers
target_include_directories(CoinGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Add Coin3D include directories - CRITICAL: Must be before PCH
if (TARGET Coin::Coin)
    # Coin::Coin target will handle includes automatically
    # Don't link here, will link later
else()
    target_include_directories(CoinGui SYSTEM PUBLIC ${COIN3D_INCLUDE_DIRS})
    target_link_directories(CoinGui PUBLIC ${COIN3D_LIB_DIRS})
endif()

# Precompiled headers - AFTER include directories are set
if(FREECAD_USE_PCH)
    target_precompile_headers(CoinGui PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

# Libraries to link
set(CoinGui_LIBS
    FreeCADGui
    Part  # Can link Part module!
)

# Add Coin3D library
if (TARGET Coin::Coin)
    list(APPEND CoinGui_LIBS Coin::Coin)
else()
    list(APPEND CoinGui_LIBS ${COIN3D_LIBRARIES})
endif()

# Link libraries
target_link_libraries(CoinGui ${CoinGui_LIBS})

# Compiler options
if (MSVC)
    target_compile_options(CoinGui PRIVATE /wd4251 /wd4273)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(CoinGui)
endif()

# Installation
SET_BIN_DIR(CoinGui CoinGui /Mod/CoinGui)
SET_PYTHON_PREFIX_SUFFIX(CoinGui)

INSTALL(TARGETS CoinGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

message(STATUS "CoinGui: Module configured")
message(STATUS "CoinGui: Coin3D includes = ${COIN3D_INCLUDE_DIRS}")
message(STATUS "CoinGui: Coin3D libs = ${COIN3D_LIBRARIES}")
