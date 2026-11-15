#include "godot/ecology/ideam_ecology_plugin.h"

namespace ideam::ecology {

void IdeamEcologyPlugin::_bind_methods() {
    using namespace godot;
    ClassDB::bind_static_method(
        "IdeamEcologyPlugin",
        D_METHOD("is_tasks_handshake_available"),
        &IdeamEcologyPlugin::is_tasks_handshake_available);
}

void IdeamEcologyPlugin::_enter_tree() {
    inspector_plugin_.instantiate();
    add_inspector_plugin(inspector_plugin_);
}

void IdeamEcologyPlugin::_exit_tree() {
    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
}

} // namespace ideam::ecology
