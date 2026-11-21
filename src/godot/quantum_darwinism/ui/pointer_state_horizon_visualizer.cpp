#include "pointer_state_horizon_visualizer.h"
#include "../ideam_quantum_darwinism_plugin.h"

#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>

using namespace godot;

namespace ideam::godot_ext {

void PointerStateHorizonVisualizer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_mode_selected", "index"), &PointerStateHorizonVisualizer::_on_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_run_pressed"), &PointerStateHorizonVisualizer::_on_run_pressed);
    ClassDB::bind_method(D_METHOD("_on_step_pressed"), &PointerStateHorizonVisualizer::_on_step_pressed);
    ClassDB::bind_method(D_METHOD("_on_reset_pressed"), &PointerStateHorizonVisualizer::_on_reset_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_channel_pressed"), &PointerStateHorizonVisualizer::_on_add_channel_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_coupling_pressed"), &PointerStateHorizonVisualizer::_on_add_coupling_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &PointerStateHorizonVisualizer::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &PointerStateHorizonVisualizer::_on_resource_changed);
}

PointerStateHorizonVisualizer::PointerStateHorizonVisualizer() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_v_size_flags(Control::SIZE_EXPAND_FILL);

    domain_tool::stamp_ui_selector(this, "qd.visualizer.root");
    domain_tool::attach_help_topic(this, get_domain_help_topic_id());
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());

    _build_ui();
}

PackedStringArray PointerStateHorizonVisualizer::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("quantum_darwinism");
    tags.append("procgen");
    tags.append("pointer_state");
    tags.append("wavefront");
    tags.append("horizon");
    return tags;
}

void PointerStateHorizonVisualizer::_build_ui() {
    // 1. Header + Toolbar
    auto *toolbar = memnew(HBoxContainer);
    toolbar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(toolbar);

    header_label_ = memnew(Label);
    header_label_->set_text("Quantum Darwinism Pointer-State & Wavefront Visualizer");
    header_label_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    toolbar->add_child(header_label_);

    mode_option_ = memnew(OptionButton);
    mode_option_->add_item("Conflict Stabilizer", 0);
    mode_option_->add_item("Outward Generative", 1);
    mode_option_->add_item("Hybrid Generate + Stabilize", 2);
    mode_option_->select(1);
    domain_tool::stamp_ui_selector(mode_option_, "qd.visualizer.mode_select");
    mode_option_->connect("item_selected", Callable(this, "_on_mode_selected"));
    toolbar->add_child(mode_option_);

    btn_step_ = memnew(Button);
    btn_step_->set_text("Step Wave");
    domain_tool::stamp_ui_selector(btn_step_, "qd.visualizer.step_btn");
    btn_step_->connect("pressed", Callable(this, "_on_step_pressed"));
    toolbar->add_child(btn_step_);

    btn_run_ = memnew(Button);
    btn_run_->set_text("Run Wavefront");
    domain_tool::stamp_ui_selector(btn_run_, "qd.visualizer.run_btn");
    btn_run_->connect("pressed", Callable(this, "_on_run_pressed"));
    toolbar->add_child(btn_run_);

    btn_reset_ = memnew(Button);
    btn_reset_->set_text("Reset");
    domain_tool::stamp_ui_selector(btn_reset_, "qd.visualizer.reset_btn");
    btn_reset_->connect("pressed", Callable(this, "_on_reset_pressed"));
    toolbar->add_child(btn_reset_);

    btn_help_ = memnew(Button);
    btn_help_->set_text("?");
    btn_help_->set_tooltip_text("Open Quantum Darwinism guided help / tutorial");
    domain_tool::stamp_ui_selector(btn_help_, "qd.visualizer.help_btn");
    btn_help_->connect("pressed", Callable(this, "_on_help_pressed"));
    toolbar->add_child(btn_help_);

    status_label_ = memnew(Label);
    status_label_->set_text("Bind a QuantumDarwinismGeneratorResource or QuantumDarwinismStabilizerResource to inspect channels & wavefronts.");
    status_label_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    add_child(status_label_);

    add_child(memnew(HSeparator));

    // 2. Main Split: Left (Channels & Couplings) | Right (Slot / Pointer-State Wavefront Tree)
    auto *split = memnew(HSplitContainer);
    split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(split);

    auto *left_col = memnew(VBoxContainer);
    left_col->set_custom_minimum_size(Vector2(260, 0));
    left_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    split->add_child(left_col);

    auto *ch_hdr = memnew(HBoxContainer);
    left_col->add_child(ch_hdr);
    auto *ch_lbl = memnew(Label);
    ch_lbl->set_text("Reflection / Semantic Channels");
    ch_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    ch_hdr->add_child(ch_lbl);

    btn_add_channel_ = memnew(Button);
    btn_add_channel_->set_text("+ Channel");
    domain_tool::stamp_ui_selector(btn_add_channel_, "qd.visualizer.add_channel_btn");
    btn_add_channel_->connect("pressed", Callable(this, "_on_add_channel_pressed"));
    ch_hdr->add_child(btn_add_channel_);

    channel_list_ = memnew(ItemList);
    channel_list_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    channel_list_->set_custom_minimum_size(Vector2(0, 120));
    domain_tool::stamp_ui_selector(channel_list_, "qd.visualizer.channel_list");
    left_col->add_child(channel_list_);

    auto *cp_hdr = memnew(HBoxContainer);
    left_col->add_child(cp_hdr);
    auto *cp_lbl = memnew(Label);
    cp_lbl->set_text("Cross-Channel Couplings");
    cp_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    cp_hdr->add_child(cp_lbl);

    btn_add_coupling_ = memnew(Button);
    btn_add_coupling_->set_text("+ Coupling");
    domain_tool::stamp_ui_selector(btn_add_coupling_, "qd.visualizer.add_coupling_btn");
    btn_add_coupling_->connect("pressed", Callable(this, "_on_add_coupling_pressed"));
    cp_hdr->add_child(btn_add_coupling_);

    coupling_list_ = memnew(ItemList);
    coupling_list_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    coupling_list_->set_custom_minimum_size(Vector2(0, 100));
    domain_tool::stamp_ui_selector(coupling_list_, "qd.visualizer.coupling_list");
    left_col->add_child(coupling_list_);

    // Right: Wavefront / Horizon Tree
    slot_wavefront_tree_ = memnew(Tree);
    slot_wavefront_tree_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    slot_wavefront_tree_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    slot_wavefront_tree_->set_columns(5);
    slot_wavefront_tree_->set_column_titles_visible(true);
    slot_wavefront_tree_->set_column_title(0, "Slot");
    slot_wavefront_tree_->set_column_title(1, "Role / State");
    slot_wavefront_tree_->set_column_title(2, "R_delta");
    slot_wavefront_tree_->set_column_title(3, "Token");
    slot_wavefront_tree_->set_column_title(4, "Channels (Locked / Value)");
    slot_wavefront_tree_->set_hide_root(true);
    domain_tool::stamp_ui_selector(slot_wavefront_tree_, "qd.visualizer.wavefront_tree");
    split->add_child(slot_wavefront_tree_);
}

void PointerStateHorizonVisualizer::bind_domain_asset(
    const Ref<Resource> &p_asset,
    const Ref<ToolContext> &p_context
) {
    if (generator_res_.is_valid() && generator_res_->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        generator_res_->disconnect("changed", Callable(this, "_on_resource_changed"));
    }
    if (stabilizer_res_.is_valid() && stabilizer_res_->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        stabilizer_res_->disconnect("changed", Callable(this, "_on_resource_changed"));
    }

    tool_context_   = p_context;
    generator_res_  = Ref<QuantumDarwinismGeneratorResource>();
    stabilizer_res_ = Ref<QuantumDarwinismStabilizerResource>();

    Ref<IdeamUndoRedo> ur = (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid())
        ? tool_context_->get_undo_redo()
        : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();

    if (p_asset.is_valid()) {
        if (auto *gen = Object::cast_to<QuantumDarwinismGeneratorResource>(p_asset.ptr())) {
            generator_res_ = Ref<QuantumDarwinismGeneratorResource>(gen);
            if (ur.is_valid()) generator_res_->propagate_undo_redo(ur);
            generator_res_->connect("changed", Callable(this, "_on_resource_changed"));
        } else if (auto *stab = Object::cast_to<QuantumDarwinismStabilizerResource>(p_asset.ptr())) {
            stabilizer_res_ = Ref<QuantumDarwinismStabilizerResource>(stab);
            if (ur.is_valid()) stabilizer_res_->set_undo_redo(ur);
            stabilizer_res_->connect("changed", Callable(this, "_on_resource_changed"));
        }
    }

    preview_initialized_ = false;
    _ensure_preview_environment();
    _refresh_ui();
}

void PointerStateHorizonVisualizer::_ensure_preview_environment() {
    if (preview_initialized_) return;
    preview_initialized_ = true;

    preview_env_.clear();
    Ref<ProceduralPattern> pat;
    if (generator_res_.is_valid()) pat = generator_res_->get_pattern();
    else if (stabilizer_res_.is_valid()) pat = stabilizer_res_->get_pattern();

    constexpr uint32_t PREVIEW_SLOTS = 12u;
    if (pat.is_valid()) {
        pat->compile_to_environment(preview_env_, PREVIEW_SLOTS);
    } else {
        preview_env_.configure_slots(PREVIEW_SLOTS, 0u);
    }

    // Build a small sample SceneTree + lateral graph so designers can step wavefronts immediately:
    // Slot 0 (Root) -> Children {1, 2, 3}; Slot 1 -> {4, 5}; Slot 2 -> {6, 7}; Slot 3 -> {8, 9, 10, 11}
    const uint32_t root_kids[3] = { 1u, 2u, 3u };
    const uint32_t b1_kids[2]   = { 4u, 5u };
    const uint32_t b2_kids[2]   = { 6u, 7u };
    const uint32_t b3_kids[4]   = { 8u, 9u, 10u, 11u };
    preview_env_.set_slot_children(0u, root_kids);
    preview_env_.set_slot_children(1u, b1_kids);
    preview_env_.set_slot_children(2u, b2_kids);
    preview_env_.set_slot_children(3u, b3_kids);

    if (generator_res_.is_valid()) {
        generator_res_->apply_to_solver(preview_solver_);
    } else if (stabilizer_res_.is_valid()) {
        preview_solver_.set_horizon_config(stabilizer_res_->to_pod());
        core::GenerativeWavefrontConfigPOD g{};
        g.mode = core::QuantumDarwinismMode::MODE_CONFLICT_STABILIZER;
        preview_solver_.set_generative_config(g);
    }

    // Seed slot 0 as an initial landmark Pointer State so outward wavefronts have an immediate anchor
    preview_solver_.bind_environment(&preview_env_);
    preview_solver_.initialize_channel_states_from_environment(preview_env_);
    if (!preview_solver_.channel_specs().empty()) {
        preview_solver_.pin_slot_channel(0u, 0u, 1.0f, 1u);
    }
}

void PointerStateHorizonVisualizer::_refresh_ui() {
    const bool is_gen = generator_res_.is_valid();
    if (mode_option_) {
        mode_option_->set_disabled(!is_gen);
        if (is_gen) mode_option_->select(generator_res_->get_solver_mode());
        else if (stabilizer_res_.is_valid()) mode_option_->select(0);
    }
    if (btn_add_channel_) btn_add_channel_->set_disabled(!is_gen);
    if (btn_add_coupling_) btn_add_coupling_->set_disabled(!is_gen);

    if (status_label_) {
        if (is_gen) {
            status_label_->set_text(
                String("Generator Preview | Wave: ") + itos(static_cast<int64_t>(preview_solver_.current_wave_index())) +
                String(" | Channels: ") + itos(static_cast<int64_t>(preview_solver_.channel_specs().size())) +
                String(" | Couplings: ") + itos(static_cast<int64_t>(preview_solver_.coupling_rules().size())) +
                String(" | Nucleations: ") + itos(static_cast<int64_t>(preview_solver_.nucleation_events().size()))
            );
        } else if (stabilizer_res_.is_valid()) {
            status_label_->set_text(
                String("Stabilizer Mode | Horizon Radius: ") + itos(stabilizer_res_->get_max_horizon_radius()) +
                String(" | Pointer Threshold: ") + String::num(stabilizer_res_->get_pointer_redundancy_threshold(), 2)
            );
        }
    }

    _refresh_channel_and_coupling_lists();
    _refresh_wavefront_tree();
}

void PointerStateHorizonVisualizer::_refresh_channel_and_coupling_lists() {
    if (!channel_list_ || !coupling_list_) return;
    channel_list_->clear();
    coupling_list_->clear();

    if (!generator_res_.is_valid()) return;

    TypedArray<QuantumDarwinismChannelSpec> chs = generator_res_->get_channels();
    for (int i = 0; i < chs.size(); ++i) {
        Ref<QuantumDarwinismChannelSpec> c = chs[i];
        if (!c.is_valid()) continue;
        const String label = String(c->get_channel_name()) +
            String(" (tau=") + String::num(c->get_default_threshold_tau(), 2) +
            String(", gamma=") + String::num(c->get_default_permeability_gamma(), 2) + String(")");
        channel_list_->add_item(label);
    }

    TypedArray<QuantumDarwinismCouplingRule> rls = generator_res_->get_coupling_rules();
    for (int i = 0; i < rls.size(); ++i) {
        Ref<QuantumDarwinismCouplingRule> r = rls[i];
        if (!r.is_valid()) continue;
        const String label = String(r->get_source_channel_name()) + String(" -> ") +
            String(r->get_target_channel_name()) +
            String(" (dTau=") + String::num(r->get_delta_threshold_tau(), 2) + String(")");
        coupling_list_->add_item(label);
    }
}

void PointerStateHorizonVisualizer::_refresh_wavefront_tree() {
    if (!slot_wavefront_tree_) return;
    slot_wavefront_tree_->clear();
    TreeItem *root = slot_wavefront_tree_->create_item();

    const auto &slots  = preview_env_.slots();
    const auto &specs  = preview_solver_.channel_specs();
    const auto &states = preview_solver_.slot_channel_states();
    const uint32_t n_ch = static_cast<uint32_t>(specs.size());

    for (size_t s = 0; s < slots.size(); ++s) {
        const auto &slot = slots[s];
        TreeItem *item = slot_wavefront_tree_->create_item(root);
        item->set_text(0, String("Slot ") + itos(static_cast<int64_t>(s)));

        bool any_pinned = (slot.locked_layer == core::PatternCascadeLayer::LAYER_PINNED_OVERRIDE);
        bool any_nucleated = false;
        uint32_t locked_ch = 0u;
        String ch_summary;
        if (n_ch > 0u && (s + 1u) * n_ch <= states.size()) {
            for (uint32_t c = 0u; c < n_ch; ++c) {
                const auto &st = states[s * n_ch + c];
                if (st.is_pinned) any_pinned = true;
                if (st.is_nucleated) any_nucleated = true;
                if (st.is_locked) ++locked_ch;
                if (c > 0u) ch_summary += " | ";
                ch_summary += String("c") + itos(static_cast<int64_t>(c)) + String(":") +
                              String::num(st.value, 2) + (st.is_locked ? String("*") : String(""));
            }
        }

        String role = "Superposition";
        Color col   = Color(0.75f, 0.78f, 0.85f);
        if (slot.has_conflict != 0u) {
            role = "Conflict";
            col  = Color(0.95f, 0.35f, 0.35f);
        } else if (any_pinned) {
            role = "Pinned Pointer";
            col  = Color(0.35f, 0.85f, 1.0f);
        } else if (any_nucleated) {
            role = "Nucleated Pointer";
            col  = Color(0.95f, 0.72f, 0.25f);
        } else if (slot.is_pointer_state != 0u || (n_ch > 0u && locked_ch == n_ch)) {
            role = "Pointer State";
            col  = Color(0.40f, 0.90f, 0.55f);
        }

        item->set_text(1, role);
        item->set_custom_color(1, col);
        item->set_text(2, String::num(slot.einselection_score, 2));
        if (slot.collapsed_token_id >= 0) {
            const std::string_view tn = preview_env_.get_token_name(static_cast<uint32_t>(slot.collapsed_token_id));
            item->set_text(3, String::utf8(tn.data(), static_cast<int>(tn.size())));
        } else {
            item->set_text(3, "-");
        }
        item->set_text(4, ch_summary);
    }
}

void PointerStateHorizonVisualizer::_on_mode_selected(int p_index) {
    if (generator_res_.is_valid()) {
        generator_res_->commit_solver_mode(p_index);
    }
}

void PointerStateHorizonVisualizer::_on_step_pressed() {
    _ensure_preview_environment();
    preview_solver_.step_outward_wavefront(preview_env_);
    preview_solver_.project_channels_to_environment(preview_env_);
    _refresh_ui();
}

void PointerStateHorizonVisualizer::_on_run_pressed() {
    preview_initialized_ = false;
    _ensure_preview_environment();
    preview_solver_.run_generative_einselection(preview_env_, nullptr);
    _refresh_ui();
}

void PointerStateHorizonVisualizer::_on_reset_pressed() {
    preview_initialized_ = false;
    _ensure_preview_environment();
    _refresh_ui();
}

void PointerStateHorizonVisualizer::_on_add_channel_pressed() {
    if (!generator_res_.is_valid()) return;
    Ref<QuantumDarwinismChannelSpec> ch;
    ch.instantiate();
    const int idx = generator_res_->get_channels().size();
    ch->set_channel_name(StringName(String("channel_") + itos(idx)));
    generator_res_->commit_add_channel(ch);
}

void PointerStateHorizonVisualizer::_on_add_coupling_pressed() {
    if (!generator_res_.is_valid()) return;
    Ref<QuantumDarwinismCouplingRule> rule;
    rule.instantiate();
    TypedArray<QuantumDarwinismChannelSpec> chs = generator_res_->get_channels();
    if (chs.size() >= 1) {
        Ref<QuantumDarwinismChannelSpec> c0 = chs[0];
        if (c0.is_valid()) rule->set_source_channel_name(c0->get_channel_name());
    }
    if (chs.size() >= 2) {
        Ref<QuantumDarwinismChannelSpec> c1 = chs[1];
        if (c1.is_valid()) rule->set_target_channel_name(c1->get_channel_name());
    }
    rule->set_delta_threshold_tau(-0.15f);
    generator_res_->commit_add_coupling_rule(rule);
}

void PointerStateHorizonVisualizer::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void PointerStateHorizonVisualizer::_on_resource_changed() {
    preview_initialized_ = false;
    _ensure_preview_environment();
    _refresh_ui();
}

} // namespace ideam::godot_ext
