#include "gameplay_mode.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>

#include "gameplay_accessibility_option.h"
#include "gameplay_audio_option.h"
#include "gameplay_control_map.h"
#include "gameplay_difficulty.h"
#include "gameplay_graphics_option.h"
#include "gameplay_perspective.h"
#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayMode::_bind_methods() {
    // Perspectives Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_perspectives", "perspectives"), &GameplayMode::set_perspectives);
    godot::ClassDB::bind_method(godot::D_METHOD("get_perspectives"), &GameplayMode::get_perspectives);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "perspectives", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayPerspective"), "set_perspectives", "get_perspectives");

    // Difficulties Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_difficulties", "difficulties"), &GameplayMode::set_difficulties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_difficulties"), &GameplayMode::get_difficulties);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "difficulties", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayDifficulty"), "set_difficulties", "get_difficulties");

    // Control Maps Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_control_maps", "control_maps"), &GameplayMode::set_control_maps);
    godot::ClassDB::bind_method(godot::D_METHOD("get_control_maps"), &GameplayMode::get_control_maps);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "control_maps", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayControlMap"), "set_control_maps", "get_control_maps");

    // Accessibility Options Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_accessibility_options", "accessibility_options"), &GameplayMode::set_accessibility_options);
    godot::ClassDB::bind_method(godot::D_METHOD("get_accessibility_options"), &GameplayMode::get_accessibility_options);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "accessibility_options", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayAccessibilityOption"), "set_accessibility_options", "get_accessibility_options");

    // Audio Options Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_audio_options", "audio_options"), &GameplayMode::set_audio_options);
    godot::ClassDB::bind_method(godot::D_METHOD("get_audio_options"), &GameplayMode::get_audio_options);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "audio_options", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayAudioOption"), "set_audio_options", "get_audio_options");

    // Graphics Options Property
    godot::ClassDB::bind_method(godot::D_METHOD("set_graphics_options", "graphics_options"), &GameplayMode::set_graphics_options);
    godot::ClassDB::bind_method(godot::D_METHOD("get_graphics_options"), &GameplayMode::get_graphics_options);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "graphics_options", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayGraphicsOption"), "set_graphics_options", "get_graphics_options");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("apply_mode", "other"), &GameplayMode::apply_mode);
}

GameplayMode::GameplayMode() {
}

GameplayMode::~GameplayMode() {
}

// Setters / Getters
void GameplayMode::set_perspectives(const godot::TypedArray<GameplayPerspective> &p_perspectives) {
    if (p_perspectives == perspectives) return;
    perspectives = p_perspectives;
    emit_changed();
}
godot::TypedArray<GameplayPerspective> GameplayMode::get_perspectives() const { return perspectives; }

void GameplayMode::set_difficulties(const godot::TypedArray<GameplayDifficulty> &p_difficulties) {
    if (p_difficulties == difficulties) return;
    difficulties = p_difficulties;
    emit_changed();
}
godot::TypedArray<GameplayDifficulty> GameplayMode::get_difficulties() const { return difficulties; }

void GameplayMode::set_control_maps(const godot::TypedArray<GameplayControlMap> &p_control_maps) {
    if (p_control_maps == control_maps) return;
    control_maps = p_control_maps;
    emit_changed();
}
godot::TypedArray<GameplayControlMap> GameplayMode::get_control_maps() const { return control_maps; }

void GameplayMode::set_accessibility_options(const godot::TypedArray<GameplayAccessibilityOption> &p_options) {
    if (p_options == accessibility_options) return;
    accessibility_options = p_options;
    emit_changed();
}
godot::TypedArray<GameplayAccessibilityOption> GameplayMode::get_accessibility_options() const { return accessibility_options; }

void GameplayMode::set_audio_options(const godot::TypedArray<GameplayAudioOption> &p_options) {
    if (p_options == audio_options) return;
    audio_options = p_options;
    emit_changed();
}
godot::TypedArray<GameplayAudioOption> GameplayMode::get_audio_options() const { return audio_options; }

void GameplayMode::set_graphics_options(const godot::TypedArray<GameplayGraphicsOption> &p_options) {
    if (p_options == graphics_options) return;
    graphics_options = p_options;
    emit_changed();
}
godot::TypedArray<GameplayGraphicsOption> GameplayMode::get_graphics_options() const { return graphics_options; }

// --- Cascade & Mode Functions ---

void GameplayMode::apply_mode(const godot::Ref<GameplayMode> &p_other) {
    if (!p_other.is_valid()) return;

    GameplayResource::apply_resource(p_other);

    // 1. Merge Perspectives
    godot::TypedArray<GameplayPerspective> other_perspectives = p_other->get_perspectives();
    for (int j = 0; j < other_perspectives.size(); ++j) {
        GameplayPerspective *op_obj = godot::Object::cast_to<GameplayPerspective>(other_perspectives[j]);
        if (!op_obj) continue;
        godot::Ref<GameplayPerspective> op_ref(op_obj);
        bool matched = false;
        for (int i = 0; i < perspectives.size(); ++i) {
            GameplayPerspective *p_obj = godot::Object::cast_to<GameplayPerspective>(perspectives[i]);
            if (p_obj && p_obj->matches_identity(op_ref)) {
                p_obj->apply_perspective(op_ref);
                matched = true;
                break;
            }
        }
        if (!matched) {
            perspectives.append(op_ref);
        }
    }

    // 2. Merge Difficulties
    godot::TypedArray<GameplayDifficulty> other_difficulties = p_other->get_difficulties();
    for (int j = 0; j < other_difficulties.size(); ++j) {
        GameplayDifficulty *od_obj = godot::Object::cast_to<GameplayDifficulty>(other_difficulties[j]);
        if (!od_obj) continue;
        godot::Ref<GameplayDifficulty> od_ref(od_obj);
        bool matched = false;
        for (int i = 0; i < difficulties.size(); ++i) {
            GameplayDifficulty *d_obj = godot::Object::cast_to<GameplayDifficulty>(difficulties[i]);
            if (d_obj && d_obj->matches_identity(od_ref)) {
                d_obj->apply_difficulty(od_ref);
                matched = true;
                break;
            }
        }
        if (!matched) {
            difficulties.append(od_ref);
        }
    }

    // 3. Merge Control Maps
    godot::TypedArray<GameplayControlMap> other_control_maps = p_other->get_control_maps();
    for (int j = 0; j < other_control_maps.size(); ++j) {
        GameplayControlMap *ocm_obj = godot::Object::cast_to<GameplayControlMap>(other_control_maps[j]);
        if (!ocm_obj) continue;
        godot::Ref<GameplayControlMap> ocm_ref(ocm_obj);
        bool matched = false;
        for (int i = 0; i < control_maps.size(); ++i) {
            GameplayControlMap *cm_obj = godot::Object::cast_to<GameplayControlMap>(control_maps[i]);
            if (cm_obj && cm_obj->matches_identity(ocm_ref)) {
                cm_obj->apply_control_map(ocm_ref);
                matched = true;
                break;
            }
        }
        if (!matched) {
            control_maps.append(ocm_ref);
        }
    }

    // 4. Merge Accessibility Options
    godot::TypedArray<GameplayAccessibilityOption> other_ao = p_other->get_accessibility_options();
    for (int j = 0; j < other_ao.size(); ++j) {
        GameplayAccessibilityOption *oao_obj = godot::Object::cast_to<GameplayAccessibilityOption>(other_ao[j]);
        if (!oao_obj) continue;
        godot::Ref<GameplayAccessibilityOption> oao_ref(oao_obj);
        bool matched = false;
        for (int i = 0; i < accessibility_options.size(); ++i) {
            GameplayAccessibilityOption *ao_obj = godot::Object::cast_to<GameplayAccessibilityOption>(accessibility_options[i]);
            if (ao_obj && ao_obj->matches_identity(oao_ref)) {
                ao_obj->apply_accessibility_option(oao_ref);
                matched = true;
                break;
            }
        }
        if (!matched) {
            accessibility_options.append(oao_ref);
        }
    }

    // 5. Merge Audio Options
    godot::TypedArray<GameplayAudioOption> other_audio = p_other->get_audio_options();
    for (int j = 0; j < other_audio.size(); ++j) {
        GameplayAudioOption *oau_obj = godot::Object::cast_to<GameplayAudioOption>(other_audio[j]);
        if (!oau_obj) continue;
        godot::Ref<GameplayAudioOption> oau_ref(oau_obj);
        bool matched = false;
        for (int i = 0; i < audio_options.size(); ++i) {
            GameplayAudioOption *au_obj = godot::Object::cast_to<GameplayAudioOption>(audio_options[i]);
            if (au_obj && au_obj->matches_identity(oau_ref)) {
                au_obj->apply_audio_option(oau_ref);
                matched = true;
                break;
            }
        }
        if (!matched) {
            audio_options.append(oau_ref);
        }
    }

    // 6. Merge Graphics Options
    godot::TypedArray<GameplayGraphicsOption> other_graphics = p_other->get_graphics_options();
    for (int j = 0; j < other_graphics.size(); ++j) {
        GameplayGraphicsOption *ogo_obj = godot::Object::cast_to<GameplayGraphicsOption>(other_graphics[j]);
        if (!ogo_obj) continue;
        godot::Ref<GameplayGraphicsOption> ogo_ref(ogo_obj);
        bool matched = false;
        for (int i = 0; i < graphics_options.size(); ++i) {
            GameplayGraphicsOption *go_obj = godot::Object::cast_to<GameplayGraphicsOption>(graphics_options[i]);
            if (go_obj && go_obj->matches_identity(ogo_ref)) {
                go_obj->apply_graphics_option(ogo_ref);
                matched = true;
                break;
            }
        }
        if (!matched) {
            graphics_options.append(ogo_ref);
        }
    }

    emit_changed();
}

godot::TypedArray<GameplayRule> GameplayMode::collect_rules() const {
    godot::TypedArray<GameplayRule> collected;
    if (!get_enabled()) {
        return collected;
    }

    // 1. Collect associated_rules on the GameplayMode itself (promoting LAYER_BASE -> LAYER_MODE)
    godot::TypedArray<GameplayRule> mode_own_rules = GameplayResource::collect_rules();
    for (int i = 0; i < mode_own_rules.size(); ++i) {
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(mode_own_rules[i]));
        if (!rule.is_valid()) continue;

        if (rule->get_cascade_layer() == RuleCascadeLayer::LAYER_BASE) {
            godot::Ref<GameplayRule> promoted(godot::Object::cast_to<GameplayRule>(rule->duplicate(true).ptr()));
            if (promoted.is_valid()) {
                promoted->set_cascade_layer(RuleCascadeLayer::LAYER_MODE);
                collected.append(promoted);
                continue;
            }
        }
        collected.append(rule);
    }

    auto append_from_options = [&collected](const godot::Array &p_options, RuleCascadeLayer p_fallback_layer) {
        for (int i = 0; i < p_options.size(); ++i) {
            GameplayResource *res = godot::Object::cast_to<GameplayResource>(p_options[i]);
            if (!res || !res->get_enabled()) continue;

            godot::TypedArray<GameplayRule> opt_rules = res->collect_rules();
            for (int j = 0; j < opt_rules.size(); ++j) {
                godot::Ref<GameplayRule> r(godot::Object::cast_to<GameplayRule>(opt_rules[j]));
                if (!r.is_valid()) continue;

                if (r->get_cascade_layer() == RuleCascadeLayer::LAYER_BASE) {
                    r->set_cascade_layer(p_fallback_layer);
                }
                collected.append(r);
            }
        }
    };

    // 2. Collect rules from all 6 domain option arrays ordered by their Cascade Layer
    append_from_options(perspectives, RuleCascadeLayer::LAYER_MODE);
    append_from_options(audio_options, RuleCascadeLayer::LAYER_MODE);
    append_from_options(graphics_options, RuleCascadeLayer::LAYER_MODE);
    append_from_options(difficulties, RuleCascadeLayer::LAYER_DIFFICULTY);
    append_from_options(control_maps, RuleCascadeLayer::LAYER_PLAYER_ACCESSIBILITY);
    append_from_options(accessibility_options, RuleCascadeLayer::LAYER_PLAYER_ACCESSIBILITY);

    return collected;
}

// --- Persistence & GameplayResource Overrides ---

void GameplayMode::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    godot::Ref<GameplayMode> other_mode = p_other;
    if (other_mode.is_valid()) {
        apply_mode(other_mode);
    } else {
        GameplayResource::apply_resource(p_other);
    }
}

godot::Dictionary GameplayMode::save_resource() const {
    godot::Dictionary data = GameplayResource::save_resource();

    auto save_array = [](const godot::Array &p_arr) {
        godot::Array out;
        for (int i = 0; i < p_arr.size(); ++i) {
            if (GameplayResource *res = godot::Object::cast_to<GameplayResource>(p_arr[i])) {
                out.append(res->save_resource());
            }
        }
        return out;
    };

    data["perspectives"] = save_array(perspectives);
    data["difficulties"] = save_array(difficulties);
    data["control_maps"] = save_array(control_maps);
    data["accessibility_options"] = save_array(accessibility_options);
    data["audio_options"] = save_array(audio_options);
    data["graphics_options"] = save_array(graphics_options);

    return data;
}

void GameplayMode::load_resource(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);

    if (p_data.has("perspectives") && p_data["perspectives"].get_type() == godot::Variant::ARRAY) {
        perspectives.clear();
        godot::Array arr = p_data["perspectives"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == godot::Variant::DICTIONARY) {
                godot::Ref<GameplayPerspective> item;
                item.instantiate();
                item->load_resource(arr[i]);
                perspectives.append(item);
            }
        }
    }

    if (p_data.has("difficulties") && p_data["difficulties"].get_type() == godot::Variant::ARRAY) {
        difficulties.clear();
        godot::Array arr = p_data["difficulties"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == godot::Variant::DICTIONARY) {
                godot::Ref<GameplayDifficulty> item;
                item.instantiate();
                item->load_resource(arr[i]);
                difficulties.append(item);
            }
        }
    }

    if (p_data.has("control_maps") && p_data["control_maps"].get_type() == godot::Variant::ARRAY) {
        control_maps.clear();
        godot::Array arr = p_data["control_maps"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == godot::Variant::DICTIONARY) {
                godot::Ref<GameplayControlMap> item;
                item.instantiate();
                item->load_resource(arr[i]);
                control_maps.append(item);
            }
        }
    }

    if (p_data.has("accessibility_options") && p_data["accessibility_options"].get_type() == godot::Variant::ARRAY) {
        accessibility_options.clear();
        godot::Array arr = p_data["accessibility_options"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == godot::Variant::DICTIONARY) {
                godot::Ref<GameplayAccessibilityOption> item;
                item.instantiate();
                item->load_resource(arr[i]);
                accessibility_options.append(item);
            }
        }
    }

    if (p_data.has("audio_options") && p_data["audio_options"].get_type() == godot::Variant::ARRAY) {
        audio_options.clear();
        godot::Array arr = p_data["audio_options"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == godot::Variant::DICTIONARY) {
                godot::Ref<GameplayAudioOption> item;
                item.instantiate();
                item->load_resource(arr[i]);
                audio_options.append(item);
            }
        }
    }

    if (p_data.has("graphics_options") && p_data["graphics_options"].get_type() == godot::Variant::ARRAY) {
        graphics_options.clear();
        godot::Array arr = p_data["graphics_options"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == godot::Variant::DICTIONARY) {
                godot::Ref<GameplayGraphicsOption> item;
                item.instantiate();
                item->load_resource(arr[i]);
                graphics_options.append(item);
            }
        }
    }

    emit_changed();
}

} // namespace ideam::godot_ext