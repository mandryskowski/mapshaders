cmake_minimum_required( VERSION 3.22 )

message( STATUS "Using CMake ${CMAKE_VERSION}" )

# Require out-of-source builds
file( TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH )

if ( EXISTS "${LOC_PATH}" )
    message( FATAL_ERROR "You cannot build in the source directory. Please use a build subdirectory." )
endif()

# Add paths to modules
list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/" )

# Turn on link time optimization for everything
#set( CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON )

# Output compile commands to compile_commands.json (for debugging CMake issues)
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Build universal lib on macOS
# Note that CMAKE_OSX_ARCHITECTURES must be set before project().
if ( APPLE )
    set( CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" )
endif()

# Main project information
project( mapshaders
    LANGUAGES
        C
        CXX
    VERSION
        0.1.0
)

# godot-cpp
# From here: https://github.com/godotengine/godot-cpp
if ( NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/extern/godot-cpp/Makefile" )
    message(
        FATAL_ERROR
            "[${PROJECT_NAME}] The godot-cpp submodule was not downloaded. Please update submodules: git submodule update --init --recursive."
    )
endif()

set( GODOT_CPP_SYSTEM_HEADERS ON CACHE BOOL "" FORCE )
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel")
set(GODOT_CPP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-cpp)

add_subdirectory(
	${GODOT_CPP_PATH}
	${CMAKE_CURRENT_BINARY_DIR}/godot-cpp 	# needed because godot-cpp is top directory
)

# Add poly2tri and polyskel libraries
add_subdirectory(extern/poly2tri)
add_subdirectory(extern/polyskel-cpp-port)

# Add libtiff
if ( APPLE )
	set(jpeg FALSE)
 	set(old-jpeg FALSE)
endif()
add_subdirectory(extern/libtiff)
set_target_properties(tiff PROPERTIES
POSITION_INDEPENDENT_CODE ON
)
if(WIN32)
set_target_properties(tiff PROPERTIES
COMPILE_OPTIONS "$<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>"
)
endif()

godot_clear_default_flags()

get_target_property(LIBRARY_SUFFIX godot-cpp LIBRARY_SUFFIX)

# Sources
add_subdirectory(src)
add_subdirectory(worldshaders)
message("Mapshaders sources are ${MAPSHADERS_SOURCES}")

# Create our library
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_executable(${PROJECT_NAME} ${MAPSHADERS_SOURCES} ${MAPSHADERS_HEADERS})
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".wasm" OUTPUT_NAME "${PROJECT_NAME}")
else()
    add_library(${PROJECT_NAME} SHARED ${MAPSHADERS_SOURCES} ${MAPSHADERS_HEADERS})
endif()


target_link_libraries( ${PROJECT_NAME}
    PUBLIC
        godot-cpp
        poly2tri
        polyskel
        tiff
)

# LIB_ARCH is the architecture being built. It is set to the build system's architecture.
# For macOS, we build a universal library (both arm64 and x86_64).
set( LIB_ARCH ${CMAKE_SYSTEM_PROCESSOR} )
if ( APPLE )
    set( LIB_ARCH "universal" )
endif()

# LIB_DIR is where the actual library ends up. This is used in both the build directory and the
# install directory and needs to be consistent with the paths in the gdextension file.
#   e.g. linux.release.x86_64 = "lib/Linux-x86_64/libGDExtensionTemplate.so"
set( LIB_DIR "lib/${CMAKE_SYSTEM_NAME}-${LIB_ARCH}" )

message( STATUS "Building ${PROJECT_NAME} for ${LIB_ARCH} on ${CMAKE_SYSTEM_NAME}")

# BUILD_OUTPUT_DIR is where we put the resulting library (in the build directory)
set( BUILD_OUTPUT_DIR "${PROJECT_BINARY_DIR}/${PROJECT_NAME}/" )

message( "Build output directory: ${BUILD_OUTPUT_DIR}")

get_target_property(GODOT_C_FLAGS godot-cpp GODOT_C_FLAGS)
get_target_property(GODOT_CXX_FLAGS godot-cpp GODOT_CXX_FLAGS)
get_target_property(GODOT_COMPILE_WARNING_FLAGS godot-cpp GODOT_COMPILE_WARNING_FLAGS)
target_compile_options(${PROJECT_NAME} PRIVATE
	${GODOT_C_FLAGS}
	${GODOT_CXX_FLAGS}
	${GODOT_COMPILE_WARNING_FLAGS}
)

set_target_properties( ${PROJECT_NAME}
    PROPERTIES
#        CXX_VISIBILITY_PRESET hidden
        RUNTIME_OUTPUT_DIRECTORY "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${BUILD_OUTPUT_DIR}/${LIB_DIR}"
)

if( NOT DEFINED CMAKE_DEBUG_POSTFIX )
    set_target_properties( ${PROJECT_NAME}
        PROPERTIES
            DEBUG_POSTFIX "-d"
    )
endif()

# Copy over additional files from the support_files directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/support_files"
            ${BUILD_OUTPUT_DIR}
)

# Warnings
include( CompilerWarnings )

# Create and include version info file from git
include( GitVersionInfo )

# Install library, extension file, and support files in ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}

set( INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/" )

message( STATUS "Install directory: ${INSTALL_DIR}")
message("Cmake lib dir is ${INSTALL_DIR} ${LIB_DIR}")
install( TARGETS ${PROJECT_NAME}
    LIBRARY
        DESTINATION ${INSTALL_DIR}/${LIB_DIR}
    RUNTIME
        DESTINATION ${INSTALL_DIR}/${LIB_DIR}
)

# Copy over support files
install( DIRECTORY "${CMAKE_SOURCE_DIR}/support_files/"
    DESTINATION ${INSTALL_DIR}
    PATTERN ".*" EXCLUDE
)

add_subdirectory( templates )

# ccache
# Turns on ccache if found
include( ccache )

# Formatting
# Adds a custom target to format all the code at once
include( ClangFormat )

#set_target_properties( godot-cpp poly2tri polyskel
#    PROPERTIES
#        CXX_VISIBILITY_PRESET hidden    # visibility needs to be the same as the main library
#)

get_target_property(GODOT_LINK_FLAGS godot-cpp GODOT_LINK_FLAGS)
if (MSVC)
    # Append /NODEFAULTLIB:msvcrt to the godot-cpp link flags
    list(APPEND GODOT_LINK_FLAGS "/WX:NO")
    message(${GODOT_LINK_FLAGS})
endif()
target_link_options(${PROJECT_NAME} PRIVATE ${GODOT_LINK_FLAGS})
