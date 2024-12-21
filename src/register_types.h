/* register_types.h */
#ifndef MS_REGISTERTYPES_H
#define MS_REGISTERTYPES_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

void initialize_mapshaders(godot::ModuleInitializationLevel p_level);
void uninitialize_mapshaders(godot::ModuleInitializationLevel p_level);

#endif  // MS_REGISTERTYPES_H