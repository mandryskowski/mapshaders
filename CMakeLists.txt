cmake_minimum_required(VERSION 3.16)
project(GDExtensionExample)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add necessary flags
set(GODOT_CPP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

# Include directories
include_directories(src/ godot-cpp/include)
add_subdirectory("${GODOT_CPP_PATH}")

# Locate source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/import/osm_parser/*.cpp"
    "src/import/elevation/*.cpp"
    "src/import/coastline/*.cpp"
    # Uncomment if you need these directories
    #"src/poly2tri/poly2tri/sweep/*.cc"
    #"src/poly2tri/poly2tri/common/*.cc"
)

# Remove specific sources (polyskel-cpp-port in your case)
list(FILTER SOURCES EXCLUDE REGEX ".*polyskel-cpp-port.*")
list(FILTER SOURCES EXCLUDE REGEX ".*poly2tri.*")

# Add poly2tri and polyskel libraries (assuming pre-built or submodule)
add_library(poly2tri STATIC IMPORTED)
set_target_properties(poly2tri PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/src/poly2tri/build/Debug/poly2tri.lib")

add_library(polyskel STATIC IMPORTED)
set_target_properties(polyskel PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/src/polyskel-cpp-port/build/Debug/polyskel.lib")

# Link directories
link_directories(src/poly2tri/build/ src/poly2tri/build/Release src/polyskel-cpp-port/build/ src/polyskel-cpp-port/build/Release)

# Create the shared library
if (APPLE)
    add_library(gdextension SHARED ${SOURCES})
    set_target_properties(gdextension PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demo/bin/libgdexample.macos.release.framework"
    )
else()
    add_library(gdextension SHARED ${SOURCES})
    set_target_properties(gdextension PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demo/bin"
        OUTPUT_NAME "libgdexample.windows.template_debug.x86_64"
        SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
endif()

# Link libraries
target_link_libraries(gdextension poly2tri polyskel godot-cpp)
