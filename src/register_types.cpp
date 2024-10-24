#include "register_types.h"
#include <godot_cpp/core/class_db.hpp>

#include "import/osm_parser/OSMParser.h"
#include "import/elevation/ElevationParser.h"
#include "import/coastline/CoastlineParser.h"


#include "import/SGImport.h"
#define P2T_STATIC_EXPORTS
#include "util/PolyUtil.h"
#include "util/GlobalRequirementsBuilder.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <iostream>

using namespace godot;

void initialize_mapshaders(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;

	WARN_PRINT_ED("Registering MapShaders types.");

	ClassDB::register_class<SGImport>();

	ClassDB::register_abstract_class<GeoMap>();
	ClassDB::register_abstract_class<OriginBasedGeoMap>();
	ClassDB::register_class<EquirectangularGeoMap>();
	ClassDB::register_class<SphereGeoMap>();

	ClassDB::register_abstract_class<TileMapBase>();
	ClassDB::register_class<EquirectangularTileMap>();

	ClassDB::register_class<ElevationGrid>();
	ClassDB::register_class<SkeletonSubtree>();
	ClassDB::register_class<PolyUtil>();
	ClassDB::register_class<GlobalRequirements>();
	ClassDB::register_class<GlobalRequirementsBuilder>();

	ClassDB::register_abstract_class<Parser>();
	ClassDB::register_class<OSMParser>();
	ClassDB::register_class<ElevationParser>();
	ClassDB::register_class<CoastlineParser>();

	ClassDB::register_abstract_class<OSMHeightmap>();
	ClassDB::register_class<ElevationHeightmap>();
}
void uninitialize_mapshaders(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT mapshaders_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	std::cout << "MapShaders init!";

	init_obj.register_initializer(initialize_mapshaders);
	init_obj.register_terminator(uninitialize_mapshaders);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}