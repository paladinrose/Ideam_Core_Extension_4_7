#include "godot/geomorphology/ideam_geomorphology_plugin.h"

namespace ideam::godot_ext {

void IdeamGeomorphologyPlugin::_bind_methods() {
    using namespace godot;
    ClassDB::bind_static_method(
        "IdeamGeomorphologyPlugin",
        D_METHOD("is_tasks_handshake_available"),
        &IdeamGeomorphologyPlugin::is_tasks_handshake_available);
}

void IdeamGeomorphologyPlugin::_enter_tree() {
    inspector_plugin_.instantiate();
    add_inspector_plugin(inspector_plugin_);
}

void IdeamGeomorphologyPlugin::_exit_tree() {
    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
}

} // namespace ideam::godot_ext
