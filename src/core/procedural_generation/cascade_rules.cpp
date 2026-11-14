#include "cascade_rules.h"
#include <algorithm>

namespace ideam::core {

uint32_t CascadeRuleEngine::compute_analysis_specificity(const AnalysisRulePOD &p_rule) noexcept {
    uint32_t base = static_cast<uint32_t>(p_rule.selector.compute_selector_specificity());
    if (p_rule.parent_rule_id != 0xFFFFFFFFu) {
        base += 50u; // Child rule refinement bonus
    }
    if (p_rule.is_important != 0u) {
        base += 10000u;
    }
    return base;
}

uint16_t CascadeRuleEngine::compute_generation_specificity(const GenerationRulePOD &p_rule) noexcept {
    uint32_t base = static_cast<uint32_t>(p_rule.selector.compute_selector_specificity());
    if (p_rule.parent_rule_id != 0xFFFFFFFFu) {
        base += 50u;
    }
    if (p_rule.required_sibling_token != 0xFFFFFFFFu || p_rule.forbidden_sibling_token != 0xFFFFFFFFu) {
        base += 40u;
    }
    return static_cast<uint16_t>(std::min<uint32_t>(65535u, base));
}

void CascadeRuleEngine::narrow_child_analysis_rule(
    const AnalysisRulePOD &p_parent,
    AnalysisRulePOD &r_child
) noexcept {
    r_child.parent_rule_id = p_parent.rule_id;
    if (r_child.selector.class_name_hash == 0u) {
        r_child.selector.class_name_hash = p_parent.selector.class_name_hash;
    }
    if (r_child.selector.object_name_hash == 0u) {
        r_child.selector.object_name_hash = p_parent.selector.object_name_hash;
    }
    if (r_child.selector.name_prefix_hash == 0u) {
        r_child.selector.name_prefix_hash = p_parent.selector.name_prefix_hash;
    }
    if (r_child.selector.name_suffix_hash == 0u) {
        r_child.selector.name_suffix_hash = p_parent.selector.name_suffix_hash;
    }
    r_child.selector.group_or_tag_mask   |= p_parent.selector.group_or_tag_mask;
    r_child.selector.property_trait_mask |= p_parent.selector.property_trait_mask;
    r_child.selector.method_trait_mask   |= p_parent.selector.method_trait_mask;
    r_child.selector.signal_trait_mask   |= p_parent.selector.signal_trait_mask;
    r_child.directives_mask              |= p_parent.directives_mask;
    if (p_parent.ignore_unmatched != 0u) {
        r_child.ignore_unmatched = 1u;
    }
    r_child.specificity = compute_analysis_specificity(r_child);
}

void CascadeRuleEngine::narrow_child_generation_rule(
    const GenerationRulePOD &p_parent,
    GenerationRulePOD &r_child
) noexcept {
    r_child.parent_rule_id = p_parent.rule_id;
    if (r_child.selector.class_name_hash == 0u) {
        r_child.selector.class_name_hash = p_parent.selector.class_name_hash;
    }
    if (r_child.selector.object_name_hash == 0u) {
        r_child.selector.object_name_hash = p_parent.selector.object_name_hash;
    }
    if (r_child.selector.name_prefix_hash == 0u) {
        r_child.selector.name_prefix_hash = p_parent.selector.name_prefix_hash;
    }
    if (r_child.selector.name_suffix_hash == 0u) {
        r_child.selector.name_suffix_hash = p_parent.selector.name_suffix_hash;
    }
    r_child.selector.group_or_tag_mask   |= p_parent.selector.group_or_tag_mask;
    r_child.selector.property_trait_mask |= p_parent.selector.property_trait_mask;
    r_child.selector.method_trait_mask   |= p_parent.selector.method_trait_mask;
    r_child.selector.signal_trait_mask   |= p_parent.selector.signal_trait_mask;
    if (p_parent.is_important != 0u) {
        r_child.is_important = 1u;
    }
    r_child.specificity = compute_generation_specificity(r_child);
}

void CascadeRuleEngine::sort_analysis_rules_by_cascade(std::span<AnalysisRulePOD> r_rules) noexcept {
    for (AnalysisRulePOD &r : r_rules) {
        if (r.specificity == 0u) {
            r.specificity = compute_analysis_specificity(r);
        }
    }
    std::stable_sort(r_rules.begin(), r_rules.end(), [](const AnalysisRulePOD &a, const AnalysisRulePOD &b) {
        if (a.is_important != b.is_important) return a.is_important < b.is_important;
        if (a.layer != b.layer) return static_cast<uint8_t>(a.layer) < static_cast<uint8_t>(b.layer);
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.specificity != b.specificity) return a.specificity < b.specificity;
        return a.rule_id < b.rule_id;
    });
}

void CascadeRuleEngine::sort_generation_rules_by_cascade(std::span<GenerationRulePOD> r_rules) noexcept {
    for (GenerationRulePOD &r : r_rules) {
        if (r.specificity == 0u) {
            r.specificity = compute_generation_specificity(r);
        }
    }
    std::stable_sort(r_rules.begin(), r_rules.end(), [](const GenerationRulePOD &a, const GenerationRulePOD &b) {
        if (a.is_important != b.is_important) return a.is_important < b.is_important;
        if (a.layer != b.layer) return static_cast<uint8_t>(a.layer) < static_cast<uint8_t>(b.layer);
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.specificity != b.specificity) return a.specificity < b.specificity;
        return a.rule_id < b.rule_id;
    });
}

ReflectionSignaturePOD CascadeRuleEngine::build_candidate_signature(
    std::string_view p_class_name,
    std::string_view p_object_name,
    std::string_view p_matched_prefix,
    std::string_view p_matched_suffix,
    uint32_t p_group_or_tag_mask,
    uint32_t p_property_mask,
    uint32_t p_method_mask,
    uint32_t p_signal_mask
) noexcept {
    ReflectionSignaturePOD sig{};
    sig.class_name_hash     = hash_reflection_symbol(p_class_name);
    sig.object_name_hash    = hash_reflection_symbol(p_object_name);
    sig.name_prefix_hash    = (!p_matched_prefix.empty() && p_object_name.starts_with(p_matched_prefix))
        ? hash_reflection_symbol(p_matched_prefix)
        : 0u;
    sig.name_suffix_hash    = (!p_matched_suffix.empty() && p_object_name.ends_with(p_matched_suffix))
        ? hash_reflection_symbol(p_matched_suffix)
        : 0u;
    sig.group_or_tag_mask   = p_group_or_tag_mask;
    sig.property_trait_mask = p_property_mask;
    sig.method_trait_mask   = p_method_mask;
    sig.signal_trait_mask   = p_signal_mask;
    return sig;
}

std::string_view CascadeRuleEngine::strip_suffix_if_present(
    std::string_view p_name,
    std::string_view p_suffix
) noexcept {
    if (!p_suffix.empty() && p_name.size() > p_suffix.size() && p_name.ends_with(p_suffix)) {
        return p_name.substr(0, p_name.size() - p_suffix.size());
    }
    return p_name;
}

} // namespace ideam::core
