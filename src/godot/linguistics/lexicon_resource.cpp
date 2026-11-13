#include "lexicon_resource.h"

namespace ideam::godot_ext {

void GrammarRuleResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_rule_name", "rule_name"), &GrammarRuleResource::set_rule_name);
    ClassDB::bind_method(D_METHOD("get_rule_name"), &GrammarRuleResource::get_rule_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "rule_name"), "set_rule_name", "get_rule_name");

    ClassDB::bind_method(D_METHOD("set_priority", "priority"), &GrammarRuleResource::set_priority);
    ClassDB::bind_method(D_METHOD("get_priority"), &GrammarRuleResource::get_priority);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
}

void LexiconResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_lexicon_name", "lexicon_name"), &LexiconResource::set_lexicon_name);
    ClassDB::bind_method(D_METHOD("get_lexicon_name"), &LexiconResource::get_lexicon_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "lexicon_name"), "set_lexicon_name", "get_lexicon_name");
}

void LanguageAssetResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_language_name", "language_name"), &LanguageAssetResource::set_language_name);
    ClassDB::bind_method(D_METHOD("get_language_name"), &LanguageAssetResource::get_language_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "language_name"), "set_language_name", "get_language_name");

    ClassDB::bind_method(D_METHOD("set_lexicons", "lexicons"), &LanguageAssetResource::set_lexicons);
    ClassDB::bind_method(D_METHOD("get_lexicons"), &LanguageAssetResource::get_lexicons);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "lexicons", PROPERTY_HINT_ARRAY_TYPE, "LexiconResource"),
        "set_lexicons",
        "get_lexicons"
    );

    ClassDB::bind_method(D_METHOD("set_grammar_rules", "grammar_rules"), &LanguageAssetResource::set_grammar_rules);
    ClassDB::bind_method(D_METHOD("get_grammar_rules"), &LanguageAssetResource::get_grammar_rules);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "grammar_rules", PROPERTY_HINT_ARRAY_TYPE, "GrammarRuleResource"),
        "set_grammar_rules",
        "get_grammar_rules"
    );
}

} // namespace ideam::godot_ext
