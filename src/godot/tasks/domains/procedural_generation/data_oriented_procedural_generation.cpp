#include "data_oriented_procedural_generation.h"

namespace ideam::godot_ext {

DataOrientedProceduralGeneration::DataOrientedProceduralGeneration()
    : dod_engine_(std::make_unique<core::ProceduralGenerationDOD>()) {}

void DataOrientedProceduralGeneration::_bind_methods() {}

} // namespace ideam::godot_ext
