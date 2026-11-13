#include "data_oriented_linguistics.h"

namespace ideam::godot_ext {

DataOrientedLinguistics::DataOrientedLinguistics()
    : dod_engine_(std::make_unique<core::LinguisticsDOD>()) {}

void DataOrientedLinguistics::_bind_methods() {}

} // namespace ideam::godot_ext
