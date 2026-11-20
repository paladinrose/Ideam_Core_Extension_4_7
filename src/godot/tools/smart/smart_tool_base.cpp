#include "smart_tool_base.h"

namespace ideam::godot_ext {

void SmartToolBase::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_tool_id", "id"), &SmartToolBase::set_tool_id);
    ClassDB::bind_method(D_METHOD("get_tool_id"), &SmartToolBase::get_tool_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tool_id"), "set_tool_id", "get_tool_id");

    ClassDB::bind_method(D_METHOD("set_default_profile", "profile"), &SmartToolBase::set_default_profile);
    ClassDB::bind_method(D_METHOD("get_default_profile"), &SmartToolBase::get_default_profile);

    ClassDB::bind_method(D_METHOD("set_user_profile", "profile"), &SmartToolBase::set_user_profile);
    ClassDB::bind_method(D_METHOD("get_user_profile"), &SmartToolBase::get_user_profile);

    ClassDB::bind_method(
        D_METHOD("resolve_effective_profile", "context"),
        &SmartToolBase::resolve_effective_profile,
        DEFVAL(Ref<ToolContext>())
    );
}

SmartToolBase::SmartToolBase() {
    default_profile_.instantiate();
}

godot::Ref<SmartToolProfile> SmartToolBase::resolve_effective_profile(
    const godot::Ref<ToolContext> &p_context
) const {
    godot::Ref<SmartToolProfile> base = default_profile_;
    if (base.is_null()) {
        base.instantiate();
        base->set_tool_id(tool_id_);
    }

    if (user_profile_.is_valid()) {
        base = base->merged_with(user_profile_);
    }

    if (p_context.is_valid() && tool_id_ != godot::StringName()) {
        godot::Ref<SmartToolProfile> ctx_override = p_context->get_preference_override(tool_id_);
        if (ctx_override.is_valid()) {
            return base->merged_with(ctx_override);
        }
    }

    return base;
}

} // namespace ideam::godot_ext
