#include "linguistics_inspector.h"

namespace ideam::godot_ext {

void LinguisticsInspector::_bind_methods() {}

bool LinguisticsInspector::_can_handle(godot::Object *p_object) const {
    if (!p_object) return false;
    return godot::Object::cast_to<LanguageAssetResource>(p_object) != nullptr ||
           godot::Object::cast_to<LexiconResource>(p_object) != nullptr ||
           godot::Object::cast_to<SememeResource>(p_object) != nullptr;
}

} // namespace ideam::godot_ext
