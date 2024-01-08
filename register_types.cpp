#include "register_types.h"
#include "core/object/class_db.h"

#include "element/SGNode.h"
#include "import/SGImport.h"

void initialize_streetsgd_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;

	ClassDB::register_class<SGNode>();
	ClassDB::register_class<SGImport>();
}
void uninitialize_streetsgd_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;
}