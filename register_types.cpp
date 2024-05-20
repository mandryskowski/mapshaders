#include "register_types.h"
#include "core/object/class_db.h"

#include "element/SGNode.h"
#include "import/SGImport.h"
#define P2T_STATIC_EXPORTS
#include "util/PolyUtil.h"
#include "util/GlobalRequirementsBuilder.h"

void initialize_streetsgd_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;

	ClassDB::register_class<SGNode>();
	ClassDB::register_class<SGImport>();
	ClassDB::register_class<PolyUtil>();
	ClassDB::register_class<GlobalRequirements>();
	ClassDB::register_class<GlobalRequirementsBuilder>();
}
void uninitialize_streetsgd_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;
}