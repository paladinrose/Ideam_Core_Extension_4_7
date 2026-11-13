#include "data_oriented_quantum_darwinism.h"

namespace ideam::godot_ext {

DataOrientedQuantumDarwinism::DataOrientedQuantumDarwinism()
    : dod_engine_(std::make_unique<core::QuantumDarwinismDOD>()) {}

void DataOrientedQuantumDarwinism::_bind_methods() {}

} // namespace ideam::godot_ext
