#include "proportion_ruler_graph.h"

#include <algorithm>
#include <queue>

namespace ideam::core {

void ProportionRulerGraph::clear() {
    nodes_.clear();
}

void ProportionRulerGraph::set_base_unit_meters(float p_meters) noexcept {
    base_unit_meters_ = std::clamp(p_meters, 0.01f, 50.0f);
    if (!nodes_.empty() && nodes_[0].parent_id == 0xFFFFFFFFu) {
        nodes_[0].computed_value = base_unit_meters_;
    }
}

uint32_t ProportionRulerGraph::add_node(
    std::string_view p_name,
    uint32_t p_parent_id,
    float p_ratio,
    float p_bias,
    bool p_is_landmark,
    float p_min_val,
    float p_max_val
) {
    const uint32_t id = static_cast<uint32_t>(nodes_.size());
    ProportionRulerNode n{};
    n.id             = id;
    n.parent_id      = p_parent_id;
    n.name           = std::string(p_name);
    n.ratio          = p_ratio;
    n.bias           = p_bias;
    n.min_value      = p_min_val;
    n.max_value      = p_max_val;
    n.is_landmark    = p_is_landmark;
    n.computed_value = (p_parent_id == 0xFFFFFFFFu) ? (base_unit_meters_ * p_ratio + p_bias) : 0.0f;
    nodes_.push_back(std::move(n));
    return id;
}

uint32_t ProportionRulerGraph::find_node_id(std::string_view p_name) const noexcept {
    for (const auto &n : nodes_) {
        if (n.name == p_name) {
            return n.id;
        }
    }
    return 0xFFFFFFFFu;
}

const ProportionRulerNode *ProportionRulerGraph::get_node(uint32_t p_id) const noexcept {
    if (p_id < nodes_.size()) return &nodes_[p_id];
    return nullptr;
}

ProportionRulerNode *ProportionRulerGraph::get_node_mut(uint32_t p_id) noexcept {
    if (p_id < nodes_.size()) return &nodes_[p_id];
    return nullptr;
}

bool ProportionRulerGraph::set_node_ratio(uint32_t p_id, float p_ratio) noexcept {
    if (p_id >= nodes_.size()) return false;
    nodes_[p_id].ratio = p_ratio;
    return true;
}

bool ProportionRulerGraph::set_node_bias(uint32_t p_id, float p_bias) noexcept {
    if (p_id >= nodes_.size()) return false;
    nodes_[p_id].bias = p_bias;
    return true;
}

bool ProportionRulerGraph::evaluate() {
    const size_t n = nodes_.size();
    if (n == 0u) return true;

    std::vector<std::vector<uint32_t>> children(n);
    std::vector<uint32_t> in_degree(n, 0u);

    for (size_t i = 0; i < n; ++i) {
        const uint32_t pid = nodes_[i].parent_id;
        if (pid != 0xFFFFFFFFu && pid < n && pid != i) {
            children[pid].push_back(static_cast<uint32_t>(i));
            in_degree[i] += 1u;
        }
    }

    std::queue<uint32_t> q;
    for (size_t i = 0; i < n; ++i) {
        if (in_degree[i] == 0u) {
            q.push(static_cast<uint32_t>(i));
        }
    }

    size_t visited = 0u;
    while (!q.empty()) {
        const uint32_t u = q.front();
        q.pop();
        visited += 1u;

        auto &node = nodes_[u];
        const float parent_val = (node.parent_id != 0xFFFFFFFFu && node.parent_id < n)
            ? nodes_[node.parent_id].computed_value
            : base_unit_meters_;
        const float raw = parent_val * node.ratio + node.bias;
        node.computed_value = std::clamp(raw, node.min_value, node.max_value);

        for (uint32_t v : children[u]) {
            if (--in_degree[v] == 0u) {
                q.push(v);
            }
        }
    }

    return visited == n;
}

float ProportionRulerGraph::get_value(uint32_t p_id, float p_fallback) const noexcept {
    if (p_id < nodes_.size()) return nodes_[p_id].computed_value;
    return p_fallback;
}

float ProportionRulerGraph::get_value_by_name(std::string_view p_name, float p_fallback) const noexcept {
    const uint32_t id = find_node_id(p_name);
    return (id != 0xFFFFFFFFu) ? nodes_[id].computed_value : p_fallback;
}

ProportionRulerGraph ProportionRulerGraph::create_loomis_biped_canon(
    LoomisBipedCanon p_preset,
    float p_head_height_m
) {
    ProportionRulerGraph g;
    g.set_unit_type(ProportionUnitType::HEAD_HEIGHT);
    g.set_base_unit_meters(p_head_height_m);

    float total_heads    = 7.5f;
    float shoulder_ratio = 2.0f;
    float hip_ratio      = 1.45f;
    float torso_ratio    = 2.6f;
    float leg_ratio      = 3.7f;

    switch (p_preset) {
        case LoomisBipedCanon::CHILD_5H:
            g.set_canon_name("Loomis Child (5H)");
            total_heads = 5.0f; shoulder_ratio = 1.45f; hip_ratio = 1.15f; torso_ratio = 1.8f; leg_ratio = 2.0f;
            break;
        case LoomisBipedCanon::IDEALISTIC_8H:
            g.set_canon_name("Loomis Idealistic (8H)");
            total_heads = 8.0f; shoulder_ratio = 2.15f; hip_ratio = 1.50f; torso_ratio = 2.75f; leg_ratio = 4.0f;
            break;
        case LoomisBipedCanon::HEROIC_8_5H:
            g.set_canon_name("Loomis Heroic (8.5H)");
            total_heads = 8.5f; shoulder_ratio = 2.45f; hip_ratio = 1.55f; torso_ratio = 2.85f; leg_ratio = 4.35f;
            break;
        case LoomisBipedCanon::STYLIZED_4_5H:
            g.set_canon_name("Loomis Stylized (4.5H)");
            total_heads = 4.5f; shoulder_ratio = 1.35f; hip_ratio = 1.10f; torso_ratio = 1.55f; leg_ratio = 1.75f;
            break;
        case LoomisBipedCanon::NORMAL_7_5H:
        default:
            g.set_canon_name("Loomis Normal (7.5H)");
            total_heads = 7.5f; shoulder_ratio = 2.0f; hip_ratio = 1.45f; torso_ratio = 2.6f; leg_ratio = 3.7f;
            break;
    }

    const uint32_t head_h = g.add_node("head_height", 0xFFFFFFFFu, 1.0f, 0.0f, false);
    const uint32_t stature = g.add_node("stature_height", head_h, total_heads, 0.0f, true);
    g.add_node("head_width",      head_h, 0.72f, 0.0f, false);
    g.add_node("head_depth",      head_h, 0.85f, 0.0f, false);
    g.add_node("neck_height",     head_h, 0.35f, 0.0f, false);
    g.add_node("shoulder_width",  head_h, shoulder_ratio, 0.0f, false);
    g.add_node("ribcage_height",  head_h, torso_ratio * 0.58f, 0.0f, false);
    g.add_node("ribcage_depth",   head_h, 1.05f, 0.0f, false);
    g.add_node("pelvis_width",    head_h, hip_ratio, 0.0f, false);
    g.add_node("pelvis_height",   head_h, torso_ratio * 0.36f, 0.0f, false);
    g.add_node("pelvis_depth",    head_h, 0.92f, 0.0f, false);
    g.add_node("upper_arm_len",   head_h, 1.35f, 0.0f, false);
    g.add_node("forearm_len",     head_h, 1.15f, 0.0f, false);
    g.add_node("hand_len",        head_h, 0.75f, 0.0f, false);
    g.add_node("thigh_len",       head_h, leg_ratio * 0.52f, 0.0f, false);
    g.add_node("shin_len",        head_h, leg_ratio * 0.44f, 0.0f, false);
    g.add_node("foot_len",        head_h, 1.05f, 0.0f, false);

    // Classic Loomis vertical landmark heights from ground (Sole = 0)
    g.add_node("landmark_crown",  stature, 1.0f, 0.0f, true);
    g.add_node("landmark_chin",   head_h,  total_heads - 1.0f, 0.0f, true);
    g.add_node("landmark_chest",  head_h,  total_heads - 2.0f, 0.0f, true);
    g.add_node("landmark_navel",  head_h,  total_heads - 3.0f, 0.0f, true);
    g.add_node("landmark_pelvis", head_h,  leg_ratio, 0.0f, true);
    g.add_node("landmark_knee",   head_h,  leg_ratio * 0.48f, 0.0f, true);
    g.add_node("landmark_ankle",  head_h,  0.30f, 0.0f, true);

    g.evaluate();
    return g;
}

ProportionRulerGraph ProportionRulerGraph::create_loomis_quadruped_canon(float p_shoulder_height_m) {
    ProportionRulerGraph g;
    g.set_unit_type(ProportionUnitType::SHOULDER_HEIGHT);
    g.set_canon_name("Loomis Quadruped (Shoulder Box)");
    g.set_base_unit_meters(p_shoulder_height_m);

    const uint32_t sh = g.add_node("shoulder_height", 0xFFFFFFFFu, 1.0f, 0.0f, true);
    g.add_node("body_length",      sh, 1.25f, 0.0f, true);
    g.add_node("ribcage_length",   sh, 0.68f, 0.0f, false);
    g.add_node("ribcage_depth",    sh, 0.46f, 0.0f, false);
    g.add_node("ribcage_width",    sh, 0.36f, 0.0f, false);
    g.add_node("pelvis_length",    sh, 0.38f, 0.0f, false);
    g.add_node("pelvis_width",     sh, 0.32f, 0.0f, false);
    g.add_node("neck_length",      sh, 0.42f, 0.0f, false);
    g.add_node("skull_length",     sh, 0.40f, 0.0f, true);
    g.add_node("foreleg_upper",    sh, 0.36f, 0.0f, false);
    g.add_node("foreleg_lower",    sh, 0.40f, 0.0f, false);
    g.add_node("hindleg_upper",    sh, 0.40f, 0.0f, false);
    g.add_node("hindleg_lower",    sh, 0.42f, 0.0f, false);
    g.add_node("tail_length",      sh, 0.72f, 0.0f, false);
    g.add_node("landmark_withers", sh, 1.0f,  0.0f, true);
    g.add_node("landmark_belly",   sh, 0.54f, 0.0f, true);
    g.add_node("landmark_elbow",   sh, 0.48f, 0.0f, true);

    g.evaluate();
    return g;
}

ProportionRulerGraph ProportionRulerGraph::create_loomis_avian_canon(float p_keel_length_m) {
    ProportionRulerGraph g;
    g.set_unit_type(ProportionUnitType::KEEL_LENGTH);
    g.set_canon_name("Loomis Avian (Keel & Wingspan)");
    g.set_base_unit_meters(p_keel_length_m);

    const uint32_t keel = g.add_node("keel_length", 0xFFFFFFFFu, 1.0f, 0.0f, true);
    g.add_node("wingspan",         keel, 3.20f, 0.0f, true);
    g.add_node("wing_inner_len",   keel, 0.75f, 0.0f, false);
    g.add_node("wing_outer_len",   keel, 0.85f, 0.0f, false);
    g.add_node("keel_width",       keel, 0.58f, 0.0f, false);
    g.add_node("keel_depth",       keel, 0.64f, 0.0f, false);
    g.add_node("neck_length",      keel, 0.48f, 0.0f, false);
    g.add_node("skull_length",     keel, 0.42f, 0.0f, true);
    g.add_node("tarsus_length",    keel, 0.62f, 0.0f, false);
    g.add_node("tail_fan_length",  keel, 0.95f, 0.0f, false);
    g.add_node("landmark_crown",   keel, 1.65f, 0.0f, true);
    g.add_node("landmark_breast",  keel, 0.90f, 0.0f, true);

    g.evaluate();
    return g;
}

ProportionRulerGraph ProportionRulerGraph::create_loomis_serpentine_canon(float p_skull_length_m) {
    ProportionRulerGraph g;
    g.set_unit_type(ProportionUnitType::SKULL_LENGTH);
    g.set_canon_name("Loomis Serpentine (Skull Rhythm)");
    g.set_base_unit_meters(p_skull_length_m);

    const uint32_t skull = g.add_node("skull_length", 0xFFFFFFFFu, 1.0f, 0.0f, true);
    g.add_node("body_length",      skull, 14.0f, 0.0f, true);
    g.add_node("skull_width",      skull, 0.65f, 0.0f, false);
    g.add_node("neck_radius",      skull, 0.24f, 0.0f, false);
    g.add_node("midbody_radius",   skull, 0.42f, 0.0f, true);
    g.add_node("tail_tip_radius",  skull, 0.06f, 0.0f, false);

    g.evaluate();
    return g;
}

ProportionRulerGraph ProportionRulerGraph::create_prop_chair_canon(float p_seat_height_m) {
    ProportionRulerGraph g;
    g.set_unit_type(ProportionUnitType::SEAT_HEIGHT);
    g.set_canon_name("Ergonomic Chair (Seat Height)");
    g.set_base_unit_meters(p_seat_height_m);

    const uint32_t seat = g.add_node("seat_height", 0xFFFFFFFFu, 1.0f, 0.0f, true);
    g.add_node("seat_width",       seat, 1.05f, 0.0f, false);
    g.add_node("seat_depth",       seat, 1.00f, 0.0f, false);
    g.add_node("backrest_height",  seat, 1.15f, 0.0f, true);
    g.add_node("armrest_height",   seat, 0.50f, 0.0f, false);
    g.add_node("leg_radius",       seat, 0.055f, 0.0f, false);

    g.evaluate();
    return g;
}

} // namespace ideam::core
