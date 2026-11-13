#include "transceiver_nodes.h"

namespace ideam::godot_ext {

void CommunicationTransmitterNode::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_elocution_skill", "skill"), &CommunicationTransmitterNode::set_elocution_skill);
    ClassDB::bind_method(D_METHOD("get_elocution_skill"), &CommunicationTransmitterNode::get_elocution_skill);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "elocution_skill"), "set_elocution_skill", "get_elocution_skill");

    ClassDB::bind_method(D_METHOD("set_projection_power_db", "power_db"), &CommunicationTransmitterNode::set_projection_power_db);
    ClassDB::bind_method(D_METHOD("get_projection_power_db"), &CommunicationTransmitterNode::get_projection_power_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "projection_power_db"), "set_projection_power_db", "get_projection_power_db");
}

void CommunicationReceiverNode::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_listen_skill", "skill"), &CommunicationReceiverNode::set_listen_skill);
    ClassDB::bind_method(D_METHOD("get_listen_skill"), &CommunicationReceiverNode::get_listen_skill);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "listen_skill"), "set_listen_skill", "get_listen_skill");

    ClassDB::bind_method(D_METHOD("set_perception_bonus_db", "bonus_db"), &CommunicationReceiverNode::set_perception_bonus_db);
    ClassDB::bind_method(D_METHOD("get_perception_bonus_db"), &CommunicationReceiverNode::get_perception_bonus_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "perception_bonus_db"), "set_perception_bonus_db", "get_perception_bonus_db");
}

} // namespace ideam::godot_ext
