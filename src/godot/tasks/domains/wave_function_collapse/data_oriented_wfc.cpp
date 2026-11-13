#include "data_oriented_wfc.h"

namespace ideam::godot_ext {

DataOrientedWaveFunctionCollapse::DataOrientedWaveFunctionCollapse()
    : dod_engine_(std::make_unique<core::WaveFunctionCollapseDOD>()) {}

void DataOrientedWaveFunctionCollapse::_bind_methods() {}

} // namespace ideam::godot_ext
