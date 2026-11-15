#include "godot/climatology/ideam_climatology_plugin.h"

namespace ideam::climatology {

void IdeamClimatologyPlugin::_bind_methods() {
    using namespace godot;
    ClassDB::bind_static_method(
        "IdeamClimatologyPlugin",
        D_METHOD("is_tasks_handshake_available"),
        &IdeamClimatologyPlugin::is_tasks_handshake_available);
}

void IdeamClimatologyPlugin::_enter_tree() {
    inspector_plugin_.instantiate();
    add_inspector_plugin(inspector_plugin_);
}

void IdeamClimatologyPlugin::_exit_tree() {
    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
}

} // namespace ideam::climatology
