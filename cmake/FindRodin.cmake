#.rst:
# Find Rodin
# -----------
#
# Finds the Rodin library. Basic usage::
#
#  find_package(Rodin REQUIRED)
#
# This module tries to find the base Rodin library and then defines the
# following:
#
#  Rodin_FOUND                 - Whether the base library was found
#
# This command will try to find only the base library, not the optional
# components. The optional components are:
#
#  Alert                - Logging, exceptions, and warnings
#  Mesh                 - Mesh data structure and methods
#  Variational          - Variational methods
#  Solver               - Linear system solving
#  Plot                 - Visualization tools [TODO: CURRENTLY BROKEN]
#
# Additionally, Rodin provides the following external components:
#
#  External::MMG
#
# Example usage with specifying additional components is:
#
#  find_package(Rodin REQUIRED Mesh Variational Solver External::MMG)
#
# For each component is then defined:
#
#  Rodin_*_FOUND               - Whether the component was found
#  Rodin::*                    - Component imported target
#  Rodin::External::*          - External component imported target
#
# The external components are currently handled the same way as components.
#
# Additionally these variables are defined for internal usage:
#
#  RODIN_INCLUDE_DIR               - Root include dir (w/o dependencies)
#  RODIN_LIBRARY                   - Rodin library (w/o dependencies)
#  RODIN_*_LIBRARY                 - Component libraries (w/o dependencies)
#  RODIN_BINARY_INSTALL_DIR        - Binary installation directory
#  RODIN_LIBRARY_INSTALL_DIR       - Library installation directory
#  RODIN_RESOURCE_INSTALL_DIR      - Data installation directory
#

# RODIN_INCLUDE_DIR
find_path(RODIN_INCLUDE_DIR
    NAMES Rodin/Rodin.h)
mark_as_advanced(RODIN_INCLUDE_DIR)

# RODIN_LIBRARY
if(NOT TARGET Rodin::Rodin)
    add_library(Rodin::Rodin UNKNOWN IMPORTED)
    find_library(RODIN_LIBRARY Rodin)

    # RODIN_INCLUDE_DIR
    set_property(TARGET Rodin::Rodin
        APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${RODIN_INCLUDE_DIR})
else()
    set(RODIN_LIBRARY Rodin::Rodin)
endif()

set(_RODIN_LIBRARY_COMPONENTS
    Core Alert Mesh Variational Plot)

set(_RODIN_EXTERNAL_COMPONENTS
    MMG)

set(_RODIN_IMPLICITLY_ENABLED_COMPONENTS
    Core)

# Inter-component dependencies
set(_RODIN_Core_DEPENDENCIES )
set(_RODIN_Alert_DEPENDENCIES )
set(_RODIN_Mesh_DEPENDENCIES )
set(_RODIN_Variational_DEPENDENCIES
    Mesh Alert)
set(_RODIN_Solver_DEPENDENCIES
    Mesh Alert Variational)

# Ensure that all inter-component dependencies are specified as well
set(_RODIN_ADDITIONAL_COMPONENTS )
foreach(_component ${Rodin_FIND_COMPONENTS})
    # Mark the dependencies as required if the component is also required, but
    # only if they themselves are not optional
    if(Rodin_FIND_REQUIRED_${_component})
        foreach(_dependency ${_RODIN_${_component}_DEPENDENCIES})
            if(NOT _RODIN_${_component}_${_dependency}_DEPENDENCY_IS_OPTIONAL)
                set(Rodin_FIND_REQUIRED_${_dependency} TRUE)
            endif()
        endforeach()
    endif()

    list(APPEND _RODIN_ADDITIONAL_COMPONENTS ${_RODIN_${_component}_DEPENDENCIES})
endforeach()

# Join the lists, remove duplicate components
set(_RODIN_ORIGINAL_FIND_COMPONENTS ${Rodin_FIND_COMPONENTS})
if(_RODIN_ADDITIONAL_COMPONENTS)
    list(INSERT Rodin_FIND_COMPONENTS 0 ${_RODIN_ADDITIONAL_COMPONENTS})
endif()
if(Rodin_FIND_COMPONENTS)
    list(REMOVE_DUPLICATES Rodin_FIND_COMPONENTS)
endif()

# Find all components. Maintain a list of components that'll need to have
# their optional dependencies checked.
set(_RODIN_OPTIONAL_DEPENDENCIES_TO_ADD )
foreach(_component ${Rodin_FIND_COMPONENTS})
    # Strip namespace from External:: components
    string(REGEX REPLACE "^External::" "" _component ${_component})

    string(TOUPPER ${_component} _COMPONENT)

    # Create imported target in case the library is found. If the project is
    # added as subproject to CMake, the target already exists and all the
    # required setup is already done from the build tree.
    if(TARGET Rodin::${_component})
        set(Rodin_${_component}_FOUND TRUE)
    else()
        # Library components
        if(_component IN_LIST _RODIN_LIBRARY_COMPONENTS)
            add_library(Rodin::${_component} UNKNOWN IMPORTED)

            # Set library defaults, find the library
            set(_RODIN_${_COMPONENT}_INCLUDE_PATH_SUFFIX Rodin/${_component})
            set(_RODIN_${_COMPONENT}_INCLUDE_PATH_NAMES ${_component}.h)

            find_library(RODIN_${_COMPONENT}_LIBRARY Rodin${_component})
        elseif(_component IN_LIST _RODIN_EXTERNAL_COMPONENTS)
            # External components
            add_library(Rodin::External::${_component} UNKNOWN IMPORTED)
            set(_RODIN_${_COMPONENT}_INCLUDE_PATH_SUFFIX RodinExternal/${_component})
            set(_RODIN_${_COMPONENT}_INCLUDE_PATH_NAMES ${_component}.h)
            find_library(RODIN_${_COMPONENT}_LIBRARY ${_component}
                PATH_SUFFIXES rodin/${_RODIN_${_COMPONENT}_PATH_SUFFIX})
        else()
            continue()
        endif()
    endif()
endforeach()
