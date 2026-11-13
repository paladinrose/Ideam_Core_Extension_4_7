#include "communications_inspector.h"

namespace ideam::godot_ext {

void CommunicationsInspector::_bind_methods() {}

bool CommunicationsInspector::_can_handle(godot::Object *p_object) const {
    if (!p_object) return false;
    return godot::Object::cast_to<CommunicationMediumResource>(p_object) != nullptr ||
           godot::Object::cast_to<DiscourseResource>(p_object) != nullptr ||
           godot::Object::cast_to<UtteranceResource>(p_object) != nullptr;
}

} // namespace ideam::godot_ext
