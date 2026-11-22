#include "character_gallery_bank_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

void CharacterGalleryBankResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(BANK_SCAFFOLD);
    BIND_ENUM_CONSTANT(BANK_TOPOLOGY);
    BIND_ENUM_CONSTANT(BANK_FEATURE);
    BIND_ENUM_CONSTANT(BANK_DETAIL);

    ADD_GROUP("Character Gallery Banks", "");

    ClassDB::bind_method(D_METHOD("set_scaffold_presets", "presets"), &CharacterGalleryBankResource::set_scaffold_presets);
    ClassDB::bind_method(D_METHOD("get_scaffold_presets"), &CharacterGalleryBankResource::get_scaffold_presets);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "scaffold_presets"), "set_scaffold_presets", "get_scaffold_presets");

    ClassDB::bind_method(D_METHOD("set_topology_presets", "presets"), &CharacterGalleryBankResource::set_topology_presets);
    ClassDB::bind_method(D_METHOD("get_topology_presets"), &CharacterGalleryBankResource::get_topology_presets);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "topology_presets"), "set_topology_presets", "get_topology_presets");

    ClassDB::bind_method(D_METHOD("set_feature_presets", "presets"), &CharacterGalleryBankResource::set_feature_presets);
    ClassDB::bind_method(D_METHOD("get_feature_presets"), &CharacterGalleryBankResource::get_feature_presets);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "feature_presets"), "set_feature_presets", "get_feature_presets");

    ClassDB::bind_method(D_METHOD("set_detail_presets", "presets"), &CharacterGalleryBankResource::set_detail_presets);
    ClassDB::bind_method(D_METHOD("get_detail_presets"), &CharacterGalleryBankResource::get_detail_presets);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "detail_presets"), "set_detail_presets", "get_detail_presets");

    ClassDB::bind_method(D_METHOD("populate_loomis_starter_banks"), &CharacterGalleryBankResource::populate_loomis_starter_banks);
    ClassDB::bind_method(
        D_METHOD("add_custom_preset", "category", "display_name", "description", "tags", "payload"),
        &CharacterGalleryBankResource::add_custom_preset
    );
    ClassDB::bind_method(
        D_METHOD("get_presets_for_bank", "category", "search_filter"),
        &CharacterGalleryBankResource::get_presets_for_bank,
        DEFVAL("")
    );
    ClassDB::bind_method(D_METHOD("find_preset_by_id", "preset_id"), &CharacterGalleryBankResource::find_preset_by_id);
    ClassDB::bind_method(
        D_METHOD("apply_preset_to_body_plan", "preset_id", "body_plan"),
        &CharacterGalleryBankResource::apply_preset_to_body_plan
    );
    ClassDB::bind_method(
        D_METHOD("apply_preset_to_character_instance", "preset_id", "instance"),
        &CharacterGalleryBankResource::apply_preset_to_character_instance
    );
}

CharacterGalleryBankResource::CharacterGalleryBankResource() {
    set_tool_id(godot::StringName("CharacterModeler"));
    set_profile_name("Loomis Starter Gallery Banks");
    set_help_topic_id(godot::StringName("CharacterGalleryBankResource"));
    populate_loomis_starter_banks();
}

godot::TypedArray<godot::Dictionary> &CharacterGalleryBankResource::_bank_ref(CharacterBankCategory p_cat) {
    switch (p_cat) {
        case BANK_TOPOLOGY: return topology_presets_;
        case BANK_FEATURE:  return feature_presets_;
        case BANK_DETAIL:   return detail_presets_;
        case BANK_SCAFFOLD:
        default:            return scaffold_presets_;
    }
}

const godot::TypedArray<godot::Dictionary> &CharacterGalleryBankResource::_bank_cref(CharacterBankCategory p_cat) const {
    switch (p_cat) {
        case BANK_TOPOLOGY: return topology_presets_;
        case BANK_FEATURE:  return feature_presets_;
        case BANK_DETAIL:   return detail_presets_;
        case BANK_SCAFFOLD:
        default:            return scaffold_presets_;
    }
}

void CharacterGalleryBankResource::set_scaffold_presets(const godot::TypedArray<godot::Dictionary> &p_presets) { scaffold_presets_ = p_presets; emit_changed(); }
void CharacterGalleryBankResource::set_topology_presets(const godot::TypedArray<godot::Dictionary> &p_presets) { topology_presets_ = p_presets; emit_changed(); }
void CharacterGalleryBankResource::set_feature_presets(const godot::TypedArray<godot::Dictionary> &p_presets)  { feature_presets_  = p_presets; emit_changed(); }
void CharacterGalleryBankResource::set_detail_presets(const godot::TypedArray<godot::Dictionary> &p_presets)   { detail_presets_   = p_presets; emit_changed(); }

godot::String CharacterGalleryBankResource::add_custom_preset(
    CharacterBankCategory p_category,
    const godot::String &p_display_name,
    const godot::String &p_description,
    const godot::PackedStringArray &p_tags,
    const godot::Dictionary &p_payload
) {
    auto &bank = _bank_ref(p_category);
    const godot::String id = godot::String("preset_") + godot::String::num_int64(static_cast<int64_t>(p_category)) + "_" + godot::String::num_int64(bank.size());
    godot::Dictionary entry;
    entry["id"]           = id;
    entry["category"]     = static_cast<int64_t>(p_category);
    entry["display_name"] = p_display_name;
    entry["description"]  = p_description;
    entry["tags"]         = p_tags;
    entry["payload"]      = p_payload;
    bank.append(entry);
    emit_changed();
    return id;
}

void CharacterGalleryBankResource::populate_loomis_starter_banks() {
    scaffold_presets_.clear();
    topology_presets_.clear();
    feature_presets_.clear();
    detail_presets_.clear();

    auto tags = [](std::initializer_list<const char *> p_list) {
        godot::PackedStringArray arr;
        for (const char *s : p_list) arr.append(s);
        return arr;
    };

    // --- 1. ScaffoldBank (Andrew Loomis Body-Plan Canons) ---
    {
        auto add_sc = [&](const char *name, const char *desc, int arch, std::initializer_list<const char *> tgs) {
            godot::Dictionary p;
            p["kind"]      = "scaffold_archetype";
            p["archetype"] = arch;
            add_custom_preset(BANK_SCAFFOLD, name, desc, tags(tgs), p);
        };
        add_sc("Loomis Normal (7.5H)",     "Classic 7.5-head humanoid figure with contrapposto & calf rhythm.", SCAFFOLD_ARCHETYPE_BIPED_NORMAL,     {"loomis", "biped", "normal"});
        add_sc("Loomis Idealistic (8H)",   "Ideal 8-head studio figure with balanced torso and stride.",        SCAFFOLD_ARCHETYPE_BIPED_IDEALISTIC, {"loomis", "biped", "ideal"});
        add_sc("Loomis Heroic (8.5H)",     "Broad-shouldered 8.5-head heroic canon.",                           SCAFFOLD_ARCHETYPE_BIPED_HEROIC,     {"loomis", "biped", "heroic"});
        add_sc("Loomis Child (5H)",        "5-head juvenile proportion canon.",                                 SCAFFOLD_ARCHETYPE_BIPED_CHILD,      {"loomis", "biped", "child"});
        add_sc("Loomis Stylized (4.5H)",   "Expressive 4.5-head stylized canon.",                               SCAFFOLD_ARCHETYPE_BIPED_STYLIZED,   {"loomis", "biped", "stylized"});
        add_sc("Loomis Quadruped",         "Shoulder-height box canon with muzzle-extended cranium.",           SCAFFOLD_ARCHETYPE_QUADRUPED,        {"loomis", "quadruped", "animal"});
        add_sc("Loomis Avian",             "Keel-length ovoid canon with 3.2x wingspan and beak.",              SCAFFOLD_ARCHETYPE_AVIAN,            {"loomis", "avian", "bird"});
        add_sc("Loomis Serpentine",        "Skull-length rhythm canon with sinusoidal girth taper.",            SCAFFOLD_ARCHETYPE_SERPENTINE,       {"loomis", "serpentine", "reptile"});
    }

    // --- 2. TopologyBank (Landmarks, Asaro Planes & Bridge Rules) ---
    {
        godot::Dictionary p1;
        p1["kind"] = "landmark"; p1["name"] = "CheekbonePlane"; p1["node_id"] = 3;
        p1["u"] = 0.17; p1["v"] = 0.44; p1["normal_offset"] = 0.010; p1["symmetry"] = 1; p1["sharpness"] = 0.65;
        add_custom_preset(BANK_TOPOLOGY, "Asaro Cheekbone Plane", "Crisp bilateral zygomatic planar break on the Loomis cranium.", tags({"asaro", "planes", "face"}), p1);

        godot::Dictionary p2;
        p2["kind"] = "landmark"; p2["name"] = "ClavicleRidge"; p2["node_id"] = 1;
        p2["u"] = 0.20; p2["v"] = 0.85; p2["normal_offset"] = 0.012; p2["symmetry"] = 1; p2["sharpness"] = 0.55;
        add_custom_preset(BANK_TOPOLOGY, "Clavicle Bony Ridge", "Bilateral collarbone landmark riding the thoracic egg.", tags({"anatomy", "torso", "landmark"}), p2);

        godot::Dictionary p3;
        p3["kind"] = "bridge"; p3["name"] = "NeckClavicleBridge"; p3["node_a"] = 1; p3["node_b"] = 2;
        p3["driving_param"] = "shoulder_width"; p3["crease_threshold"] = 2.10; p3["deep_fold_threshold"] = 2.45;
        add_custom_preset(BANK_TOPOLOGY, "Neck-Clavicle Bridge Rule", "Regime-switching neck/trapezius fold rule.", tags({"bridge", "neck", "fold"}), p3);
    }

    // --- 3. FeatureBank (Parametric Sub-Patches) ---
    {
        godot::Dictionary f1;
        f1["kind"] = "feature"; f1["name"] = "ElvenEar"; f1["feature_kind"] = 1; f1["landmark_id"] = 3;
        f1["extents"] = godot::Vector3(0.016f, 0.048f, 0.012f); f1["symmetry"] = 1; f1["secondary"] = 0.75;
        add_custom_preset(BANK_FEATURE, "Pointed Elven Ear", "Elongated pointed ear anchored to the temporal plane.", tags({"ear", "fantasy", "head"}), f1);

        godot::Dictionary f2;
        f2["kind"] = "feature"; f2["name"] = "CurvedHorn"; f2["feature_kind"] = 4; f2["landmark_id"] = 1;
        f2["extents"] = godot::Vector3(0.022f, 0.022f, 0.085f); f2["symmetry"] = 1; f2["secondary"] = 0.45;
        add_custom_preset(BANK_FEATURE, "Bilateral Ram Horns", "Tapered bilateral horns anchored to the brow keystone.", tags({"horn", "fantasy", "creature"}), f2);

        godot::Dictionary f3;
        f3["kind"] = "feature"; f3["name"] = "RaptorBeak"; f3["feature_kind"] = 6; f3["landmark_id"] = 2;
        f3["extents"] = godot::Vector3(0.020f, 0.018f, 0.045f); f3["symmetry"] = 0; f3["secondary"] = 0.55;
        add_custom_preset(BANK_FEATURE, "Hooked Raptor Beak", "Sharp tapered avian beak anchored to the nasal bridge.", tags({"beak", "avian", "head"}), f3);
    }

    // --- 4. DetailBank (Layer 4 Unconstrained Asymmetric Deltas) ---
    {
        godot::Dictionary d1;
        d1["kind"] = "detail"; d1["label"] = "LeftBrowRaise"; d1["node_id"] = 3;
        d1["u"] = 0.19; d1["v"] = 0.63; d1["delta"] = godot::Vector3(0.010f, 0.0f, 0.014f); d1["radius"] = 0.045; d1["crease"] = false;
        add_custom_preset(BANK_DETAIL, "Asymmetric Left Brow Raise", "Skeptical single-brow lift (symmetry forced OFF).", tags({"asymmetric", "expression", "face"}), d1);

        godot::Dictionary d2;
        d2["kind"] = "detail"; d2["label"] = "CrookedNose"; d2["node_id"] = 3;
        d2["u"] = 0.25; d2["v"] = 0.45; d2["delta"] = godot::Vector3(0.004f, 0.012f, 0.0f); d2["radius"] = 0.040; d2["crease"] = false;
        add_custom_preset(BANK_DETAIL, "Crooked Nose Bridge", "Subtle lateral boxer's nose deflection.", tags({"asymmetric", "nose", "character"}), d2);

        godot::Dictionary d3;
        d3["kind"] = "detail"; d3["label"] = "CheekScarCrease"; d3["node_id"] = 3;
        d3["u"] = 0.31; d3["v"] = 0.44; d3["delta"] = godot::Vector3(-0.012f, 0.0f, 0.0f); d3["radius"] = 0.032; d3["crease"] = true;
        add_custom_preset(BANK_DETAIL, "Right Cheek Scar Crease", "Sharp recessed battle-scar crease on the right cheek.", tags({"asymmetric", "scar", "crease"}), d3);
    }
}

godot::TypedArray<godot::Dictionary> CharacterGalleryBankResource::get_presets_for_bank(
    CharacterBankCategory p_category,
    const godot::String &p_search_filter
) const {
    const auto &bank = _bank_cref(p_category);
    if (p_search_filter.strip_edges().is_empty()) {
        return bank;
    }

    const godot::String needle = p_search_filter.strip_edges().to_lower();
    godot::TypedArray<godot::Dictionary> filtered;
    for (int64_t i = 0; i < bank.size(); ++i) {
        const godot::Dictionary d = bank[i];
        const godot::String name = godot::String(d.get("display_name", "")).to_lower();
        const godot::String desc = godot::String(d.get("description", "")).to_lower();
        bool match = name.contains(needle) || desc.contains(needle);
        if (!match) {
            const godot::PackedStringArray tgs = d.get("tags", godot::PackedStringArray());
            for (int64_t t = 0; t < tgs.size(); ++t) {
                if (tgs[t].to_lower().contains(needle)) {
                    match = true;
                    break;
                }
            }
        }
        if (match) {
            filtered.append(d);
        }
    }
    return filtered;
}

godot::Dictionary CharacterGalleryBankResource::find_preset_by_id(const godot::String &p_preset_id) const {
    for (int c = 0; c < 4; ++c) {
        const auto &bank = _bank_cref(static_cast<CharacterBankCategory>(c));
        for (int64_t i = 0; i < bank.size(); ++i) {
            const godot::Dictionary d = bank[i];
            if (godot::String(d.get("id", "")) == p_preset_id) {
                return d;
            }
        }
    }
    return godot::Dictionary();
}

bool CharacterGalleryBankResource::apply_preset_to_body_plan(
    const godot::String &p_preset_id,
    const godot::Ref<BodyPlanResource> &p_plan
) const {
    if (!p_plan.is_valid()) return false;
    const godot::Dictionary preset = find_preset_by_id(p_preset_id);
    if (preset.is_empty()) return false;

    const godot::Dictionary payload = preset.get("payload", godot::Dictionary());
    const godot::String kind = payload.get("kind", "");

    if (kind == "scaffold_archetype") {
        const int arch = int(payload.get("archetype", 0));
        if (p_plan->get_base_scaffold().is_valid()) {
            p_plan->get_base_scaffold()->set_archetype(static_cast<FormScaffoldArchetype>(arch));
            return true;
        }
    } else if (kind == "landmark") {
        p_plan->add_topology_landmark(
            godot::String(payload.get("name", "Landmark")),
            int(payload.get("node_id", 0)),
            static_cast<real_t>(double(payload.get("u", 0.5))),
            static_cast<real_t>(double(payload.get("v", 0.5))),
            static_cast<real_t>(double(payload.get("normal_offset", 0.0))),
            int(payload.get("symmetry", 1)),
            static_cast<real_t>(double(payload.get("sharpness", 0.35)))
        );
        return true;
    } else if (kind == "bridge") {
        p_plan->add_bridge_rule(
            godot::String(payload.get("name", "BridgeRule")),
            int(payload.get("node_a", 0)),
            int(payload.get("node_b", 1)),
            godot::String(payload.get("driving_param", "")),
            static_cast<real_t>(double(payload.get("crease_threshold", 0.5))),
            static_cast<real_t>(double(payload.get("deep_fold_threshold", 0.8)))
        );
        return true;
    } else if (kind == "feature") {
        p_plan->add_feature(
            godot::String(payload.get("name", "Feature")),
            int(payload.get("feature_kind", 0)),
            int(payload.get("landmark_id", 0)),
            godot::Vector3(payload.get("extents", godot::Vector3(0.02f, 0.02f, 0.02f))),
            int(payload.get("symmetry", 1)),
            static_cast<real_t>(double(payload.get("secondary", 0.0)))
        );
        return true;
    }
    return false;
}

bool CharacterGalleryBankResource::apply_preset_to_character_instance(
    const godot::String &p_preset_id,
    const godot::Ref<CharacterInstanceResource> &p_instance
) const {
    if (!p_instance.is_valid()) return false;
    const godot::Dictionary preset = find_preset_by_id(p_preset_id);
    if (preset.is_empty()) return false;

    const godot::Dictionary payload = preset.get("payload", godot::Dictionary());
    if (godot::String(payload.get("kind", "")) == "detail") {
        p_instance->add_detail_delta(
            godot::String(payload.get("label", "Detail")),
            int(payload.get("node_id", 0)),
            static_cast<real_t>(double(payload.get("u", 0.5))),
            static_cast<real_t>(double(payload.get("v", 0.5))),
            godot::Vector3(payload.get("delta", godot::Vector3())),
            static_cast<real_t>(double(payload.get("radius", 0.04))),
            bool(payload.get("crease", false))
        );
        return true;
    }
    return false;
}

} // namespace ideam::godot_ext
