#include "data_oriented_communications.h"

namespace ideam::godot_ext {

DataOrientedCommunications::DataOrientedCommunications()
    : dod_engine_(std::make_unique<core::CommunicationsDOD>()) {}

void DataOrientedCommunications::_bind_methods() {}

} // namespace ideam::godot_ext
