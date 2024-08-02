#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

dirs = ['import', 'import/osm_parser', 'util']

env.Append(CPPPATH=["src/"])
sources = Glob("*src/*.cpp")
sources.extend(Glob("src/**/*.cpp"))
sources.extend(Glob("src/import/osm_parser/*.cpp"))
#for dir in dirs:
#    sources.extend(Glob("src/" + dir + "/*.cpp"))
#sources.extend(Glob("#streetsgd/*.cpp"))
#sources.extend(Glob("src/poly2tri/poly2tri/sweep/*.cc"))
#sources.extend(Glob("src/poly2tri/poly2tri/common/*.cc"))

env.Append(LIBPATH=["src/poly2tri/build"])
env.Append(LIBS=["poly2tri"])

for x in sources:
    print(x)


if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/libgdexample.{}.{}.framework/libgdexample.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "demo/bin/libgdexample{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,

    )

Default(library)