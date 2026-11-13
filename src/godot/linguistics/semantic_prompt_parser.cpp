#include "semantic_prompt_parser.h"

namespace ideam::godot_ext {

void SemanticPromptParser::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_language_asset", "language_asset"), &SemanticPromptParser::set_language_asset);
    ClassDB::bind_method(D_METHOD("get_language_asset"), &SemanticPromptParser::get_language_asset);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "language_asset", PROPERTY_HINT_RESOURCE_TYPE, "LanguageAssetResource"),
        "set_language_asset",
        "get_language_asset"
    );

    ClassDB::bind_method(
        D_METHOD("parse_prompt_to_tag_set", "prompt"),
        &SemanticPromptParser::parse_prompt_to_tag_set
    );
}

godot::Ref<SemanticTagSetResource> SemanticPromptParser::parse_prompt_to_tag_set(
    const godot::String &p_prompt
) const {
    (void)p_prompt;
    godot::Ref<SemanticTagSetResource> tag_set;
    tag_set.instantiate();
    // Architectural stub: Morphological + Sememe lookup of prompt tokens will populate tag_set here.
    return tag_set;
}

} // namespace ideam::godot_ext
