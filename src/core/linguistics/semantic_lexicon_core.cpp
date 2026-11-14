#include "semantic_lexicon_core.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>

namespace ideam::core {

namespace {

[[nodiscard]] std::string normalize_ascii_token(std::string_view p_input) {
    std::string out;
    out.reserve(p_input.size());
    for (char c : p_input) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 'A' && uc <= 'Z') {
            out.push_back(static_cast<char>(uc + ('a' - 'A')));
        } else if (uc == '-' || uc == ' ') {
            if (!out.empty() && out.back() != '_') {
                out.push_back('_');
            }
        } else if ((uc >= 'a' && uc <= 'z') || (uc >= '0' && uc <= '9') || uc == '_') {
            out.push_back(static_cast<char>(uc));
        }
    }
    while (!out.empty() && out.front() == '_') {
        out.erase(out.begin());
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }
    return out;
}

struct CanonicalSemeMapping {
    std::string_view tag;
    uint32_t bit;
};

constexpr std::array<CanonicalSemeMapping, 134> CANONICAL_SEME_TABLE = {{
    // Spatial & Dimensional (0..11)
    {"1d", SEME_BIT_DIM_1D},
    {"dim_1d", SEME_BIT_DIM_1D},
    {"spline_1d", SEME_BIT_DIM_1D},
    {"2d", SEME_BIT_DIM_2D},
    {"dim_2d", SEME_BIT_DIM_2D},
    {"flat", SEME_BIT_DIM_2D},
    {"canvas", SEME_BIT_DIM_2D},
    {"3d", SEME_BIT_DIM_3D},
    {"dim_3d", SEME_BIT_DIM_3D},
    {"spatial_3d", SEME_BIT_DIM_3D},
    {"4d", SEME_BIT_DIM_4D},
    {"dim_4d", SEME_BIT_DIM_4D},
    {"top_down", SEME_BIT_SPATIAL_TOP_DOWN},
    {"spatial_top_down", SEME_BIT_SPATIAL_TOP_DOWN},
    {"overhead", SEME_BIT_SPATIAL_TOP_DOWN},
    {"first_person", SEME_BIT_SPATIAL_FIRST_PERS},
    {"spatial_first_person", SEME_BIT_SPATIAL_FIRST_PERS},
    {"fps", SEME_BIT_SPATIAL_FIRST_PERS},
    {"third_person", SEME_BIT_SPATIAL_THIRD_PERS},
    {"spatial_third_person", SEME_BIT_SPATIAL_THIRD_PERS},
    {"tps", SEME_BIT_SPATIAL_THIRD_PERS},
    {"isometric", SEME_BIT_SPATIAL_ISOMETRIC},
    {"spatial_isometric", SEME_BIT_SPATIAL_ISOMETRIC},
    {"side_scroller", SEME_BIT_SPATIAL_SIDE_VIEW},
    {"side_view", SEME_BIT_SPATIAL_SIDE_VIEW},
    {"platformer", SEME_BIT_SPATIAL_SIDE_VIEW},
    {"vr", SEME_BIT_SPATIAL_XR_VR},
    {"xr", SEME_BIT_SPATIAL_XR_VR},
    {"spatial_xr_immersive", SEME_BIT_SPATIAL_XR_VR},
    {"openxr", SEME_BIT_SPATIAL_XR_VR},
    {"immersive", SEME_BIT_SPATIAL_XR_VR},
    {"interior", SEME_BIT_SPATIAL_INTERIOR},
    {"indoor", SEME_BIT_SPATIAL_INTERIOR},
    {"dungeon", SEME_BIT_SPATIAL_INTERIOR},
    {"exterior", SEME_BIT_SPATIAL_EXTERIOR},
    {"outdoor", SEME_BIT_SPATIAL_EXTERIOR},
    {"overworld", SEME_BIT_SPATIAL_EXTERIOR},

    // Era & World Setting (12..21)
    {"ancient", SEME_BIT_ERA_ANCIENT},
    {"prehistoric", SEME_BIT_ERA_ANCIENT},
    {"medieval", SEME_BIT_ERA_MEDIEVAL},
    {"era_medieval", SEME_BIT_ERA_MEDIEVAL},
    {"feudal", SEME_BIT_ERA_MEDIEVAL},
    {"renaissance", SEME_BIT_ERA_RENAISSANCE},
    {"industrial", SEME_BIT_ERA_INDUSTRIAL},
    {"steampunk", SEME_BIT_ERA_INDUSTRIAL},
    {"victorian", SEME_BIT_ERA_INDUSTRIAL},
    {"modern", SEME_BIT_ERA_MODERN},
    {"contemporary", SEME_BIT_ERA_MODERN},
    {"futuristic", SEME_BIT_ERA_FUTURISTIC},
    {"future", SEME_BIT_ERA_FUTURISTIC},
    {"cyberpunk", SEME_BIT_ERA_FUTURISTIC},
    {"fantasy", SEME_BIT_GENRE_FANTASY},
    {"genre_fantasy", SEME_BIT_GENRE_FANTASY},
    {"magic", SEME_BIT_GENRE_FANTASY},
    {"arcane", SEME_BIT_GENRE_FANTASY},
    {"wizard", SEME_BIT_GENRE_FANTASY},
    {"mage", SEME_BIT_GENRE_FANTASY},
    {"sorcerer", SEME_BIT_GENRE_FANTASY},
    {"spell", SEME_BIT_GENRE_FANTASY},
    {"scifi", SEME_BIT_GENRE_SCIFI},
    {"sci_fi", SEME_BIT_GENRE_SCIFI},
    {"science_fiction", SEME_BIT_GENRE_SCIFI},
    {"space", SEME_BIT_GENRE_SCIFI},
    {"gothic", SEME_BIT_GENRE_GOTHIC_HORROR},
    {"horror", SEME_BIT_GENRE_GOTHIC_HORROR},
    {"dark", SEME_BIT_GENRE_GOTHIC_HORROR},
    {"mystery", SEME_BIT_GENRE_MYSTERY},
    {"detective", SEME_BIT_GENRE_MYSTERY},
    {"investigation", SEME_BIT_GENRE_MYSTERY},

    // Mechanics (22..29)
    {"rpg", SEME_BIT_MECHANIC_RPG},
    {"mechanic_rpg", SEME_BIT_MECHANIC_RPG},
    {"system_rpg", SEME_BIT_MECHANIC_RPG},
    {"roleplaying", SEME_BIT_MECHANIC_RPG},
    {"action", SEME_BIT_MECHANIC_ACTION},
    {"adventure", SEME_BIT_MECHANIC_ACTION},
    {"strategy", SEME_BIT_MECHANIC_STRATEGY},
    {"tactical", SEME_BIT_MECHANIC_STRATEGY},
    {"board", SEME_BIT_MECHANIC_STRATEGY},
    {"turn_based", SEME_BIT_MECHANIC_TURN_BASED},
    {"real_time", SEME_BIT_MECHANIC_REAL_TIME},
    {"puzzle", SEME_BIT_MECHANIC_PUZZLE},
    {"simulation", SEME_BIT_MECHANIC_SIMULATION},
    {"procedural", SEME_BIT_MECHANIC_PROCEDURAL},
    {"wfc", SEME_BIT_MECHANIC_PROCEDURAL},

    // Ontology & Roles (30..43)
    {"agent", SEME_BIT_ONTOLOGY_AGENT},
    {"character", SEME_BIT_ONTOLOGY_AGENT},
    {"player", SEME_BIT_ONTOLOGY_AGENT},
    {"npc", SEME_BIT_ONTOLOGY_AGENT},
    {"creature", SEME_BIT_ONTOLOGY_CREATURE},
    {"monster", SEME_BIT_ONTOLOGY_CREATURE},
    {"prop", SEME_BIT_ONTOLOGY_PROP_ITEM},
    {"item", SEME_BIT_ONTOLOGY_PROP_ITEM},
    {"piece", SEME_BIT_ONTOLOGY_PROP_ITEM},
    {"weapon", SEME_BIT_ONTOLOGY_WEAPON},
    {"blade", SEME_BIT_ONTOLOGY_WEAPON},
    {"garment", SEME_BIT_ONTOLOGY_GARMENT},
    {"apparel", SEME_BIT_ONTOLOGY_GARMENT},
    {"armor", SEME_BIT_ONTOLOGY_GARMENT},
    {"structure", SEME_BIT_ONTOLOGY_STRUCTURE},
    {"building", SEME_BIT_ONTOLOGY_STRUCTURE},
    {"castle", SEME_BIT_ONTOLOGY_STRUCTURE},
    {"fortress", SEME_BIT_ONTOLOGY_STRUCTURE},
    {"wall", SEME_BIT_ONTOLOGY_STRUCTURE},
    {"room", SEME_BIT_ONTOLOGY_ROOM_SPACE},
    {"cell", SEME_BIT_ONTOLOGY_ROOM_SPACE},
    {"grounds", SEME_BIT_ONTOLOGY_ROOM_SPACE},
    {"landing", SEME_BIT_ONTOLOGY_ROOM_SPACE},
    {"aperture", SEME_BIT_ONTOLOGY_APERTURE},
    {"door", SEME_BIT_ONTOLOGY_APERTURE},
    {"gate", SEME_BIT_ONTOLOGY_APERTURE},
    {"window", SEME_BIT_ONTOLOGY_APERTURE},
    {"portal", SEME_BIT_ONTOLOGY_APERTURE},
    {"vertical", SEME_BIT_ONTOLOGY_VERTICAL},
    {"stairs", SEME_BIT_ONTOLOGY_VERTICAL},
    {"tower", SEME_BIT_ONTOLOGY_VERTICAL},
    {"ladder", SEME_BIT_ONTOLOGY_VERTICAL},
    {"container", SEME_BIT_ONTOLOGY_CONTAINER},
    {"chest", SEME_BIT_ONTOLOGY_CONTAINER},
    {"hazard", SEME_BIT_ONTOLOGY_HAZARD},
    {"trap", SEME_BIT_ONTOLOGY_HAZARD},
    {"light", SEME_BIT_ONTOLOGY_LIGHT},
    {"audio", SEME_BIT_ONTOLOGY_AUDIO},
    {"sound", SEME_BIT_ONTOLOGY_AUDIO},
    {"sensor", SEME_BIT_ONTOLOGY_SENSOR},

    // Capabilities & States (44..63)
    {"movable", SEME_BIT_CAP_MOVABLE},
    {"interactive", SEME_BIT_CAP_INTERACTIVE},
    {"communicative", SEME_BIT_CAP_COMMUNICATIVE},
    {"combat", SEME_BIT_CAP_COMBAT},
    {"lockable", SEME_BIT_CAP_LOCKABLE},
    {"destructible", SEME_BIT_CAP_DESTRUCTIBLE},
    {"active", SEME_BIT_STATE_ACTIVE},
    {"locked", SEME_BIT_STATE_LOCKED},
    {"damaged", SEME_BIT_STATE_DAMAGED},
    {"hidden", SEME_BIT_STATE_HIDDEN}
}};

struct ARPAbetEntry {
    std::string_view arpabet;
    uint32_t ipa_codepoint;
    uint16_t mask;
    uint16_t duration_ms;
    uint8_t  sonority;
};

/**
 * Complete 39-phoneme CMU Pronouncing Dictionary (CMUdict) ARPAbet -> IPA & 16-bit PanPhon
 * ArticulatoryFeatureBits mapping table.
 */
constexpr std::array<ARPAbetEntry, 39> ARPABET_TABLE = {{
    // Vowels & Diphthongs
    {"IY", 0x0069u, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_HIGH, 110, 9},
    {"IH", 0x026Au, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_HIGH, 85, 9},
    {"EY", 0x0065u, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED, 115, 9},
    {"EH", 0x025Bu, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED, 90, 9},
    {"AE", 0x00E6u, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_LOW, 115, 10},
    {"AA", 0x0251u, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_LOW | FEAT_VOWEL_BACK, 120, 10},
    {"AO", 0x0254u, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_LOW | FEAT_VOWEL_BACK | FEAT_VOWEL_ROUND, 115, 9},
    {"OW", 0x006Fu, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_BACK | FEAT_VOWEL_ROUND, 115, 9},
    {"UH", 0x028Au, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_HIGH | FEAT_VOWEL_BACK | FEAT_VOWEL_ROUND, 85, 9},
    {"UW", 0x0075u, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_HIGH | FEAT_VOWEL_BACK | FEAT_VOWEL_ROUND, 110, 9},
    {"AH", 0x028Cu, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_BACK, 80, 9},
    {"ER", 0x025Du, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_CONTINUANT | FEAT_CORONAL, 110, 8},
    {"AY", 0x0251u, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_LOW | FEAT_VOWEL_HIGH, 130, 10},
    {"AW", 0x0251u, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_LOW | FEAT_VOWEL_BACK | FEAT_VOWEL_ROUND, 130, 10},
    {"OY", 0x0254u, FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED | FEAT_VOWEL_BACK | FEAT_VOWEL_ROUND | FEAT_VOWEL_HIGH, 130, 9},

    // Stops
    {"P",  0x0070u, FEAT_LABIAL, 70, 1},
    {"B",  0x0062u, FEAT_VOICED | FEAT_LABIAL, 70, 2},
    {"T",  0x0074u, FEAT_CORONAL | FEAT_ANTERIOR, 65, 1},
    {"D",  0x0064u, FEAT_VOICED | FEAT_CORONAL | FEAT_ANTERIOR, 65, 2},
    {"K",  0x006Bu, FEAT_DORSAL, 75, 1},
    {"G",  0x0067u, FEAT_VOICED | FEAT_DORSAL, 75, 2},

    // Affricates
    {"CH", 0x02A7u, FEAT_STRIDENT | FEAT_CORONAL, 95, 3},
    {"JH", 0x02A4u, FEAT_VOICED | FEAT_STRIDENT | FEAT_CORONAL, 95, 4},

    // Fricatives
    {"F",  0x0066u, FEAT_CONTINUANT | FEAT_LABIAL, 90, 3},
    {"V",  0x0076u, FEAT_VOICED | FEAT_CONTINUANT | FEAT_LABIAL, 85, 4},
    {"TH", 0x03B8u, FEAT_CONTINUANT | FEAT_CORONAL | FEAT_ANTERIOR, 90, 3},
    {"DH", 0x00F0u, FEAT_VOICED | FEAT_CONTINUANT | FEAT_CORONAL | FEAT_ANTERIOR, 80, 4},
    {"S",  0x0073u, FEAT_CONTINUANT | FEAT_STRIDENT | FEAT_CORONAL | FEAT_ANTERIOR, 100, 3},
    {"Z",  0x007Au, FEAT_VOICED | FEAT_CONTINUANT | FEAT_STRIDENT | FEAT_CORONAL | FEAT_ANTERIOR, 95, 4},
    {"SH", 0x0283u, FEAT_CONTINUANT | FEAT_STRIDENT | FEAT_CORONAL, 105, 3},
    {"ZH", 0x0292u, FEAT_VOICED | FEAT_CONTINUANT | FEAT_STRIDENT | FEAT_CORONAL, 95, 4},
    {"HH", 0x0068u, FEAT_CONTINUANT | FEAT_GLOTTAL, 75, 5},

    // Nasals
    {"M",  0x006Du, FEAT_SONORANT | FEAT_VOICED | FEAT_NASAL | FEAT_LABIAL, 80, 6},
    {"N",  0x006Eu, FEAT_SONORANT | FEAT_VOICED | FEAT_NASAL | FEAT_CORONAL | FEAT_ANTERIOR, 75, 6},
    {"NG", 0x014Bu, FEAT_SONORANT | FEAT_VOICED | FEAT_NASAL | FEAT_DORSAL, 85, 6},

    // Liquids & Glides
    {"L",  0x006Cu, FEAT_SONORANT | FEAT_VOICED | FEAT_CONTINUANT | FEAT_LATERAL | FEAT_CORONAL | FEAT_ANTERIOR, 75, 7},
    {"R",  0x0279u, FEAT_SONORANT | FEAT_VOICED | FEAT_CONTINUANT | FEAT_CORONAL, 75, 7},
    {"W",  0x0077u, FEAT_SONORANT | FEAT_VOICED | FEAT_CONTINUANT | FEAT_VOWEL_HIGH | FEAT_VOWEL_BACK | FEAT_VOWEL_ROUND | FEAT_LABIAL, 70, 8},
    {"Y",  0x006Au, FEAT_SONORANT | FEAT_VOICED | FEAT_CONTINUANT | FEAT_VOWEL_HIGH | FEAT_DORSAL, 70, 8}
}};

[[nodiscard]] constexpr float weighted_phoneme_substitution_cost(uint16_t p_mask_a, uint16_t p_mask_b) noexcept {
    const uint16_t diff = static_cast<uint16_t>(p_mask_a ^ p_mask_b);
    if (diff == 0u) return 0.0f;
    const uint32_t tier1 = static_cast<uint32_t>(std::popcount(static_cast<uint16_t>(diff & FEAT_TIER1_PROSODIC_MASK)));
    const uint32_t tier2 = static_cast<uint32_t>(std::popcount(static_cast<uint16_t>(diff & FEAT_TIER2_VOWEL_MASK)));
    const uint32_t tier3 = static_cast<uint32_t>(std::popcount(static_cast<uint16_t>(diff & FEAT_TIER3_MANNER_MASK)));
    const uint32_t tier4 = static_cast<uint32_t>(std::popcount(static_cast<uint16_t>(diff & FEAT_TIER4_PLACE_MASK)));
    return (1.5f * static_cast<float>(tier1) +
            1.2f * static_cast<float>(tier2) +
            1.0f * static_cast<float>(tier3) +
            0.60f * static_cast<float>(tier4));
}

} // namespace

void SemanticLexiconCore::clear() noexcept {
    semes_.clear();
    sememes_.clear();
    phonemes_.clear();
    morphemes_.clear();
    lexemes_.clear();
    lects_.clear();
    semantic_frames_.clear();
    synset_relations_.clear();
    bpe_vocab_.clear();
    bpe_merges_.clear();
    lexeme_phoneme_feature_csr_.clear();
    lexeme_morpheme_id_csr_.clear();
    morpheme_phoneme_feature_csr_.clear();
    seme_names_.clear();
    sememe_labels_.clear();
    morpheme_forms_.clear();
    lexeme_surfaces_.clear();
    frame_names_.clear();
    bpe_pieces_.clear();
}

uint16_t SemanticLexiconCore::arpabet_to_articulatory_mask(
    std::string_view p_arpabet,
    uint8_t *r_out_stress
) noexcept {
    if (p_arpabet.empty()) return FEAT_NONE;

    char cleaned[4] = {'\0', '\0', '\0', '\0'};
    size_t len = 0;
    uint8_t stress = 0;

    for (char c : p_arpabet) {
        if (c >= '0' && c <= '2') {
            stress = static_cast<uint8_t>(c - '0');
        } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            if (len < 3) {
                cleaned[len++] = static_cast<char>((c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c);
            }
        }
    }
    if (r_out_stress) {
        *r_out_stress = stress;
    }
    std::string_view base_sym(cleaned, len);
    for (const auto &entry : ARPABET_TABLE) {
        if (entry.arpabet == base_sym) {
            return entry.mask;
        }
    }
    return FEAT_NONE;
}

void SemanticLexiconCore::populate_default_english_ipa_phonemes() {
    if (!phonemes_.empty()) {
        return;
    }
    for (const auto &row : ARPABET_TABLE) {
        register_phoneme(row.ipa_codepoint, row.mask, row.duration_ms, row.sonority, 0, row.arpabet);
    }
}

void SemanticLexiconCore::populate_default_core_semes_and_morphemes() {
    populate_default_english_ipa_phonemes();

    for (const auto &entry : CANONICAL_SEME_TABLE) {
        register_seme(entry.tag, entry.bit, static_cast<uint16_t>(SemanticDomain::GENERAL), 0, 0xFFFFu, true);
    }

    // Standard English derivational & inflectional morphemes
    register_morpheme("un",   MorphemeKind::BOUND_PREFIX, SEME_BIT_AFFECT_NEGATIVE, 1ULL << SEME_BIT_AFFECT_NEGATIVE, true,  UniversalPOS::UNDEFINED);
    register_morpheme("non",  MorphemeKind::BOUND_PREFIX, SEME_BIT_AFFECT_NEGATIVE, 1ULL << SEME_BIT_AFFECT_NEGATIVE, true,  UniversalPOS::UNDEFINED);
    register_morpheme("in",   MorphemeKind::BOUND_PREFIX, SEME_BIT_AFFECT_NEGATIVE, 1ULL << SEME_BIT_AFFECT_NEGATIVE, true,  UniversalPOS::UNDEFINED);
    register_morpheme("im",   MorphemeKind::BOUND_PREFIX, SEME_BIT_AFFECT_NEGATIVE, 1ULL << SEME_BIT_AFFECT_NEGATIVE, true,  UniversalPOS::UNDEFINED);
    register_morpheme("dis",  MorphemeKind::BOUND_PREFIX, SEME_BIT_AFFECT_NEGATIVE, 1ULL << SEME_BIT_AFFECT_NEGATIVE, true,  UniversalPOS::UNDEFINED);
    register_morpheme("re",   MorphemeKind::BOUND_PREFIX, SEME_BIT_STATE_ACTIVE,    1ULL << SEME_BIT_STATE_ACTIVE,    false, UniversalPOS::VERB);
    register_morpheme("pre",  MorphemeKind::BOUND_PREFIX, SEME_BIT_STATE_ACTIVE,    0ULL,                             false, UniversalPOS::UNDEFINED);
    register_morpheme("over", MorphemeKind::BOUND_PREFIX, SEME_BIT_SPATIAL_TOP_DOWN,1ULL << SEME_BIT_SPATIAL_TOP_DOWN,false, UniversalPOS::UNDEFINED);
    register_morpheme("under",MorphemeKind::BOUND_PREFIX, SEME_BIT_SPATIAL_INTERIOR,1ULL << SEME_BIT_SPATIAL_INTERIOR,false, UniversalPOS::UNDEFINED);

    register_morpheme("less", MorphemeKind::BOUND_SUFFIX, SEME_BIT_AFFECT_NEGATIVE, 1ULL << SEME_BIT_AFFECT_NEGATIVE, true,  UniversalPOS::ADJECTIVE);
    register_morpheme("able", MorphemeKind::BOUND_SUFFIX, SEME_BIT_CAP_INTERACTIVE, 1ULL << SEME_BIT_CAP_INTERACTIVE, false, UniversalPOS::ADJECTIVE);
    register_morpheme("ible", MorphemeKind::BOUND_SUFFIX, SEME_BIT_CAP_INTERACTIVE, 1ULL << SEME_BIT_CAP_INTERACTIVE, false, UniversalPOS::ADJECTIVE);
    register_morpheme("ful",  MorphemeKind::BOUND_SUFFIX, SEME_BIT_AFFECT_POSITIVE, 1ULL << SEME_BIT_AFFECT_POSITIVE, false, UniversalPOS::ADJECTIVE);
    register_morpheme("ous",  MorphemeKind::BOUND_SUFFIX, SEME_BIT_STATE_ACTIVE,    0ULL,                             false, UniversalPOS::ADJECTIVE);
    register_morpheme("ive",  MorphemeKind::BOUND_SUFFIX, SEME_BIT_CAP_INTERACTIVE, 1ULL << SEME_BIT_CAP_INTERACTIVE, false, UniversalPOS::ADJECTIVE);
    register_morpheme("ness", MorphemeKind::BOUND_SUFFIX, SEME_BIT_STATE_ACTIVE,    0ULL,                             false, UniversalPOS::NOUN);
    register_morpheme("tion", MorphemeKind::BOUND_SUFFIX, SEME_BIT_STATE_ACTIVE,    0ULL,                             false, UniversalPOS::NOUN);
    register_morpheme("ment", MorphemeKind::BOUND_SUFFIX, SEME_BIT_STATE_ACTIVE,    0ULL,                             false, UniversalPOS::NOUN);
    register_morpheme("er",   MorphemeKind::BOUND_SUFFIX, SEME_BIT_ONTOLOGY_AGENT,  1ULL << SEME_BIT_ONTOLOGY_AGENT,  false, UniversalPOS::NOUN);
    register_morpheme("or",   MorphemeKind::BOUND_SUFFIX, SEME_BIT_ONTOLOGY_AGENT,  1ULL << SEME_BIT_ONTOLOGY_AGENT,  false, UniversalPOS::NOUN);
    register_morpheme("ly",   MorphemeKind::BOUND_SUFFIX, SEME_BIT_STATE_ACTIVE,    0ULL,                             false, UniversalPOS::ADVERB);
    register_morpheme("ing",  MorphemeKind::INFLECTIONAL_AFFIX, SEME_BIT_STATE_ACTIVE, 1ULL << SEME_BIT_STATE_ACTIVE, false, UniversalPOS::VERB);
    register_morpheme("ed",   MorphemeKind::INFLECTIONAL_AFFIX, SEME_BIT_STATE_ACTIVE, 0ULL,                          false, UniversalPOS::ADJECTIVE);
    register_morpheme("es",   MorphemeKind::INFLECTIONAL_AFFIX, SEME_BIT_STATE_ACTIVE, 0ULL,                          false, UniversalPOS::NOUN);
    register_morpheme("s",    MorphemeKind::INFLECTIONAL_AFFIX, SEME_BIT_STATE_ACTIVE, 0ULL,                          false, UniversalPOS::NOUN);

    // Default FrameNet-style core SemanticFrames
    SemanticFramePOD attack_frame{};
    attack_frame.required_agent_seme_mask32    = (1u << (SEME_BIT_ONTOLOGY_AGENT & 31u)) | (1u << (SEME_BIT_ONTOLOGY_CREATURE & 31u));
    attack_frame.required_patient_seme_mask32  = (1u << (SEME_BIT_CAP_DESTRUCTIBLE & 31u)) | (1u << (SEME_BIT_ONTOLOGY_AGENT & 31u));
    attack_frame.required_instrument_mask32    = (1u << (SEME_BIT_ONTOLOGY_WEAPON & 31u));
    attack_frame.consequent_state_seme_mask32  = (1u << (SEME_BIT_STATE_DAMAGED & 31u));
    attack_frame.valence_polarity              = 32;
    register_semantic_frame("Attack", attack_frame);

    SemanticFramePOD open_closure_frame{};
    open_closure_frame.required_agent_seme_mask32   = (1u << SEME_BIT_ONTOLOGY_AGENT);
    open_closure_frame.required_patient_seme_mask32 = (1u << (SEME_BIT_ONTOLOGY_APERTURE & 31u)) | (1u << (SEME_BIT_ONTOLOGY_CONTAINER & 31u));
    open_closure_frame.valence_polarity             = 160;
    register_semantic_frame("Closure_Open", open_closure_frame);

    SemanticFramePOD communication_frame{};
    communication_frame.required_agent_seme_mask32   = (1u << SEME_BIT_ONTOLOGY_AGENT);
    communication_frame.required_patient_seme_mask32 = (1u << SEME_BIT_ONTOLOGY_AGENT);
    communication_frame.valence_polarity             = 140;
    register_semantic_frame("Communication_Statement", communication_frame);
}

void SemanticLexiconCore::merge_from(const SemanticLexiconCore &p_other) {
    if (&p_other == this) {
        return;
    }
    if (phonemes_.empty() && !p_other.phonemes_.empty()) {
        phonemes_ = p_other.phonemes_;
    }
    for (size_t i = 0; i < p_other.semes_.size() && i < p_other.seme_names_.size(); ++i) {
        const auto &s = p_other.semes_[i];
        register_seme(p_other.seme_names_[i], s.seme_id, s.domain_id, s.dimension_id, s.antonym_seme_id, s.is_nuclear_default != 0);
    }
    for (size_t i = 0; i < p_other.morphemes_.size() && i < p_other.morpheme_forms_.size(); ++i) {
        const auto &m = p_other.morphemes_[i];
        std::span<const uint16_t> ph_slice{};
        if (m.phoneme_csr_offset + m.phoneme_count <= p_other.morpheme_phoneme_feature_csr_.size()) {
            ph_slice = std::span<const uint16_t>(
                p_other.morpheme_phoneme_feature_csr_.data() + m.phoneme_csr_offset,
                m.phoneme_count
            );
        }
        register_morpheme(
            p_other.morpheme_forms_[i],
            m.kind,
            m.primary_seme_id,
            m.seme_modifier_mask,
            m.negates_root != 0,
            static_cast<UniversalPOS>(m.category_shift),
            ph_slice
        );
    }
    for (size_t i = 0; i < p_other.sememes_.size() && i < p_other.sememe_labels_.size(); ++i) {
        register_sememe(p_other.sememe_labels_[i], p_other.sememes_[i]);
    }
    for (size_t i = 0; i < p_other.lexemes_.size() && i < p_other.lexeme_surfaces_.size(); ++i) {
        const auto &lx = p_other.lexemes_[i];
        std::string_view sememe_label = p_other.get_sememe_label(lx.sememe_id);
        int32_t mapped_sememe = find_sememe_by_label(sememe_label);
        uint32_t target_sememe_id = (mapped_sememe >= 0) ? static_cast<uint32_t>(mapped_sememe) : lx.sememe_id;
        std::span<const uint16_t> ph_slice = p_other.get_lexeme_phoneme_masks(static_cast<uint32_t>(i));
        register_lexeme(
            p_other.lexeme_surfaces_[i],
            target_sememe_id,
            lx.category,
            lx.lect_id,
            lx.register_formality,
            lx.corpus_frequency,
            lx.slm_bpe_token_id,
            ph_slice
        );
    }
    for (const auto &lect : p_other.lects_) {
        register_lect(lect);
    }
    for (size_t i = 0; i < p_other.semantic_frames_.size() && i < p_other.frame_names_.size(); ++i) {
        register_semantic_frame(p_other.frame_names_[i], p_other.semantic_frames_[i]);
    }
    for (const auto &rel : p_other.synset_relations_) {
        register_synset_relation(rel.source_sememe_id, rel.target_sememe_id, rel.relation, rel.relation_weight);
    }
    for (size_t i = 0; i < p_other.bpe_vocab_.size() && i < p_other.bpe_pieces_.size(); ++i) {
        const auto &bv = p_other.bpe_vocab_[i];
        register_bpe_vocab_entry(p_other.bpe_pieces_[i], bv.bpe_token_id, bv.unigram_score, bv.mapped_sememe_id);
    }
    for (const auto &bm : p_other.bpe_merges_) {
        register_bpe_merge_rule(bm.left_token_id, bm.right_token_id, bm.merged_token_id, bm.priority_rank);
    }
}

uint32_t SemanticLexiconCore::resolve_canonical_seme_bit(std::string_view p_tag_name) noexcept {
    std::string norm = normalize_ascii_token(p_tag_name);
    if (norm.empty()) {
        return 0u;
    }
    std::string_view view(norm);
    for (const auto &entry : CANONICAL_SEME_TABLE) {
        if (view == entry.tag) {
            return entry.bit;
        }
    }
    return hash_to_dynamic_seme_bit(hash_linguistic_token(view));
}

uint32_t SemanticLexiconCore::resolve_seme_bit(std::string_view p_tag_name) const noexcept {
    std::string norm = normalize_ascii_token(p_tag_name);
    if (norm.empty()) {
        return 0u;
    }
    const uint32_t h = hash_linguistic_token(norm);
    for (size_t i = 0; i < semes_.size(); ++i) {
        if (semes_[i].name_hash == h && i < seme_names_.size() && seme_names_[i] == norm) {
            return semes_[i].seme_id;
        }
    }
    return resolve_canonical_seme_bit(norm);
}

uint32_t SemanticLexiconCore::register_seme(
    std::string_view p_tag_name,
    uint32_t p_preferred_bit,
    uint16_t p_domain_id,
    uint16_t p_dimension_id,
    uint16_t p_antonym_seme_id,
    bool p_is_nuclear_default
) {
    std::string norm = normalize_ascii_token(p_tag_name);
    if (norm.empty()) {
        return 0u;
    }
    const uint32_t h = hash_linguistic_token(norm);
    for (size_t i = 0; i < semes_.size(); ++i) {
        if (semes_[i].name_hash == h && i < seme_names_.size() && seme_names_[i] == norm) {
            return semes_[i].seme_id;
        }
    }
    const uint32_t bit = (p_preferred_bit < SEME_BIT_CAPACITY)
                             ? p_preferred_bit
                             : resolve_canonical_seme_bit(norm);
    SemeDescriptorPOD pod{};
    pod.seme_id            = bit;
    pod.name_hash          = h;
    pod.domain_id          = p_domain_id;
    pod.dimension_id       = p_dimension_id;
    pod.antonym_seme_id    = p_antonym_seme_id;
    pod.is_nuclear_default = p_is_nuclear_default ? 1u : 0u;
    semes_.push_back(pod);
    seme_names_.push_back(std::move(norm));
    return bit;
}

uint32_t SemanticLexiconCore::register_phoneme(
    uint32_t p_ipa_codepoint,
    uint16_t p_articulatory_mask,
    uint16_t p_duration_ms,
    uint8_t p_sonority_rank,
    uint8_t p_stress_level,
    std::string_view p_arpabet_code
) {
    for (const auto &ph : phonemes_) {
        if (ph.ipa_codepoint == p_ipa_codepoint &&
            ph.articulatory_mask == p_articulatory_mask &&
            ph.stress_level == p_stress_level) {
            return ph.phoneme_id;
        }
    }
    PhonemePOD pod{};
    pod.phoneme_id        = static_cast<uint32_t>(phonemes_.size());
    pod.ipa_codepoint     = p_ipa_codepoint;
    pod.articulatory_mask = p_articulatory_mask;
    pod.duration_ms       = p_duration_ms;
    pod.sonority_rank     = p_sonority_rank;
    pod.stress_level      = p_stress_level;
    if (!p_arpabet_code.empty()) {
        pod.arpabet_code[0] = p_arpabet_code[0];
        if (p_arpabet_code.size() > 1) {
            pod.arpabet_code[1] = p_arpabet_code[1];
        }
    }
    phonemes_.push_back(pod);
    return pod.phoneme_id;
}

uint16_t SemanticLexiconCore::find_phoneme_mask_by_codepoint(uint32_t p_ipa_codepoint) const noexcept {
    for (const auto &ph : phonemes_) {
        if (ph.ipa_codepoint == p_ipa_codepoint) {
            return ph.articulatory_mask;
        }
    }
    return FEAT_NONE;
}

uint32_t SemanticLexiconCore::register_sememe(
    std::string_view p_canonical_label,
    const SememeDescriptorPOD &p_descriptor
) {
    std::string norm = normalize_ascii_token(p_canonical_label);
    const uint32_t h = (p_descriptor.canonical_name_hash != 0u)
                           ? p_descriptor.canonical_name_hash
                           : hash_linguistic_token(norm);

    for (size_t i = 0; i < sememes_.size(); ++i) {
        if (i < sememe_labels_.size() && sememe_labels_[i] == norm) {
            sememes_[i].union_with(p_descriptor);
            sememes_[i].emotive_valence   = p_descriptor.emotive_valence;
            sememes_[i].sememic_form      = p_descriptor.sememic_form;
            sememes_[i].taxeme_category   = p_descriptor.taxeme_category;
            sememes_[i].primary_domain_id = p_descriptor.primary_domain_id;
            return sememes_[i].sememe_id;
        }
    }

    SememeDescriptorPOD copy = p_descriptor;
    copy.sememe_id           = static_cast<uint32_t>(sememes_.size());
    copy.canonical_name_hash = h;

    // If no MRL binary embedding was explicitly provided in words[1..2], populate a
    // deterministic character n-gram LSH SimHash so morphologically/orthographically related
    // terms share bit similarity in MRL space automatically!
    if (!copy.has_mrl_binary_embedding() && !norm.empty()) {
        const auto simhash = compute_character_ngram_simhash128(norm);
        copy.set_mrl_binary_embedding_128(simhash[0], simhash[1]);
    }

    sememes_.push_back(copy);
    sememe_labels_.push_back(std::move(norm));
    return copy.sememe_id;
}

int32_t SemanticLexiconCore::find_sememe_by_label(std::string_view p_label) const noexcept {
    std::string norm = normalize_ascii_token(p_label);
    if (norm.empty()) {
        return -1;
    }
    for (size_t i = 0; i < sememes_.size() && i < sememe_labels_.size(); ++i) {
        if (sememe_labels_[i] == norm) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

std::string_view SemanticLexiconCore::get_sememe_label(uint32_t p_sememe_id) const noexcept {
    if (p_sememe_id < sememe_labels_.size()) {
        return sememe_labels_[p_sememe_id];
    }
    return {};
}

uint32_t SemanticLexiconCore::register_morpheme(
    std::string_view p_form,
    MorphemeKind p_kind,
    uint16_t p_primary_seme_id,
    uint64_t p_seme_modifier_mask,
    bool p_negates_root,
    UniversalPOS p_category_shift,
    std::span<const uint16_t> p_phoneme_masks
) {
    std::string norm = normalize_ascii_token(p_form);
    const uint32_t h = hash_linguistic_token(norm);
    for (size_t i = 0; i < morphemes_.size(); ++i) {
        if (morphemes_[i].form_string_hash == h &&
            morphemes_[i].kind == p_kind &&
            i < morpheme_forms_.size() &&
            morpheme_forms_[i] == norm) {
            return morphemes_[i].morpheme_id;
        }
    }

    std::vector<uint16_t> auto_phonemes;
    std::span<const uint16_t> active_phonemes = p_phoneme_masks;
    if (active_phonemes.empty()) {
        approximate_phonemes_from_text(norm, auto_phonemes);
        active_phonemes = auto_phonemes;
    }

    MorphemePOD pod{};
    pod.morpheme_id        = static_cast<uint32_t>(morphemes_.size());
    pod.form_string_hash   = h;
    pod.phoneme_csr_offset = static_cast<uint32_t>(morpheme_phoneme_feature_csr_.size());
    pod.phoneme_count      = static_cast<uint16_t>(active_phonemes.size());
    pod.primary_seme_id    = p_primary_seme_id;
    pod.seme_modifier_mask = p_seme_modifier_mask;
    pod.kind               = p_kind;
    pod.negates_root       = p_negates_root ? 1u : 0u;
    pod.category_shift     = static_cast<uint8_t>(p_category_shift);

    morpheme_phoneme_feature_csr_.insert(
        morpheme_phoneme_feature_csr_.end(),
        active_phonemes.begin(),
        active_phonemes.end()
    );
    morphemes_.push_back(pod);
    morpheme_forms_.push_back(std::move(norm));
    return pod.morpheme_id;
}

uint32_t SemanticLexiconCore::register_lexeme(
    std::string_view p_surface_form,
    uint32_t p_sememe_id,
    UniversalPOS p_category,
    uint16_t p_lect_id,
    uint8_t p_register_formality,
    float p_corpus_frequency,
    uint32_t p_slm_bpe_token_id,
    std::span<const uint16_t> p_explicit_phoneme_masks,
    std::span<const uint32_t> p_morpheme_ids
) {
    std::string norm = normalize_ascii_token(p_surface_form);
    for (size_t i = 0; i < lexemes_.size(); ++i) {
        if (i < lexeme_surfaces_.size() &&
            lexeme_surfaces_[i] == norm &&
            lexemes_[i].lect_id == p_lect_id) {
            lexemes_[i].sememe_id          = p_sememe_id;
            lexemes_[i].category           = p_category;
            lexemes_[i].register_formality = p_register_formality;
            lexemes_[i].corpus_frequency   = p_corpus_frequency;
            if (p_slm_bpe_token_id != 0xFFFFFFFFu) {
                lexemes_[i].slm_bpe_token_id = p_slm_bpe_token_id;
            }
            if (!p_explicit_phoneme_masks.empty()) {
                lexemes_[i].phoneme_csr_offset = static_cast<uint32_t>(lexeme_phoneme_feature_csr_.size());
                lexemes_[i].phoneme_count      = static_cast<uint16_t>(p_explicit_phoneme_masks.size());
                lexeme_phoneme_feature_csr_.insert(
                    lexeme_phoneme_feature_csr_.end(),
                    p_explicit_phoneme_masks.begin(),
                    p_explicit_phoneme_masks.end()
                );
            }
            return lexemes_[i].lexeme_id;
        }
    }

    std::vector<uint16_t> auto_phonemes;
    std::span<const uint16_t> active_phonemes = p_explicit_phoneme_masks;
    if (active_phonemes.empty()) {
        approximate_phonemes_from_text(norm, auto_phonemes);
        active_phonemes = auto_phonemes;
    }

    LexemePOD pod{};
    pod.lexeme_id           = static_cast<uint32_t>(lexemes_.size());
    pod.sememe_id           = p_sememe_id;
    pod.morpheme_csr_offset = static_cast<uint32_t>(lexeme_morpheme_id_csr_.size());
    pod.morpheme_count      = static_cast<uint16_t>(p_morpheme_ids.size());
    pod.lect_id             = p_lect_id;
    pod.phoneme_csr_offset  = static_cast<uint32_t>(lexeme_phoneme_feature_csr_.size());
    pod.phoneme_count       = static_cast<uint16_t>(active_phonemes.size());
    pod.category            = p_category;
    pod.register_formality  = p_register_formality;
    pod.corpus_frequency    = p_corpus_frequency;
    pod.slm_bpe_token_id    = p_slm_bpe_token_id;

    lexeme_morpheme_id_csr_.insert(
        lexeme_morpheme_id_csr_.end(),
        p_morpheme_ids.begin(),
        p_morpheme_ids.end()
    );
    lexeme_phoneme_feature_csr_.insert(
        lexeme_phoneme_feature_csr_.end(),
        active_phonemes.begin(),
        active_phonemes.end()
    );
    lexemes_.push_back(pod);
    lexeme_surfaces_.push_back(std::move(norm));
    return pod.lexeme_id;
}

int32_t SemanticLexiconCore::find_lexeme_by_surface(
    std::string_view p_surface_form,
    uint16_t p_preferred_lect_id
) const noexcept {
    std::string norm = normalize_ascii_token(p_surface_form);
    if (norm.empty()) {
        return -1;
    }
    int32_t fallback_match = -1;
    for (size_t i = 0; i < lexemes_.size() && i < lexeme_surfaces_.size(); ++i) {
        if (lexeme_surfaces_[i] == norm) {
            if (lexemes_[i].lect_id == p_preferred_lect_id) {
                return static_cast<int32_t>(i);
            }
            if (fallback_match < 0) {
                fallback_match = static_cast<int32_t>(i);
            }
        }
    }
    return fallback_match;
}

std::string_view SemanticLexiconCore::get_lexeme_surface(uint32_t p_lexeme_id) const noexcept {
    if (p_lexeme_id < lexeme_surfaces_.size()) {
        return lexeme_surfaces_[p_lexeme_id];
    }
    return {};
}

std::span<const uint16_t> SemanticLexiconCore::get_lexeme_phoneme_masks(uint32_t p_lexeme_id) const noexcept {
    if (p_lexeme_id >= lexemes_.size()) {
        return {};
    }
    const auto &lx = lexemes_[p_lexeme_id];
    if (lx.phoneme_csr_offset + lx.phoneme_count > lexeme_phoneme_feature_csr_.size()) {
        return {};
    }
    return std::span<const uint16_t>(
        lexeme_phoneme_feature_csr_.data() + lx.phoneme_csr_offset,
        lx.phoneme_count
    );
}

uint32_t SemanticLexiconCore::register_lect(const LectDescriptorPOD &p_lect) {
    for (size_t i = 0; i < lects_.size(); ++i) {
        if (lects_[i].lect_id == p_lect.lect_id) {
            lects_[i] = p_lect;
            return static_cast<uint32_t>(i);
        }
    }
    lects_.push_back(p_lect);
    return static_cast<uint32_t>(lects_.size() - 1);
}

// ============================================================================
// WordNet Synset Relations & FrameNet Semantic Frames
// ============================================================================

uint32_t SemanticLexiconCore::register_synset_relation(
    uint32_t p_source_sememe_id,
    uint32_t p_target_sememe_id,
    WordNetRelationType p_relation,
    float p_weight
) {
    for (size_t i = 0; i < synset_relations_.size(); ++i) {
        if (synset_relations_[i].source_sememe_id == p_source_sememe_id &&
            synset_relations_[i].target_sememe_id == p_target_sememe_id &&
            synset_relations_[i].relation == p_relation) {
            synset_relations_[i].relation_weight = p_weight;
            return static_cast<uint32_t>(i);
        }
    }
    SynsetRelationPOD rel{};
    rel.source_sememe_id = p_source_sememe_id;
    rel.target_sememe_id = p_target_sememe_id;
    rel.relation         = p_relation;
    rel.relation_weight  = p_weight;
    synset_relations_.push_back(rel);

    // Automatically propagate symbolic ontology bits from Hypernym (`target`) to Hyponym (`source`)!
    if (p_relation == WordNetRelationType::WN_HYPERNYM &&
        p_source_sememe_id < sememes_.size() &&
        p_target_sememe_id < sememes_.size()) {
        sememes_[p_source_sememe_id].nuclear_seme_words[0] |=
            sememes_[p_target_sememe_id].nuclear_seme_words[0];
    }
    return static_cast<uint32_t>(synset_relations_.size() - 1);
}

bool SemanticLexiconCore::is_hypernym_of(
    uint32_t p_candidate_hypernym_id,
    uint32_t p_hyponym_sememe_id,
    uint32_t p_max_hops
) const noexcept {
    if (p_candidate_hypernym_id == p_hyponym_sememe_id) {
        return true;
    }
    std::array<uint32_t, 32> frontier{};
    std::array<uint32_t, 32> next_frontier{};
    size_t frontier_count = 1;
    frontier[0] = p_hyponym_sememe_id;

    for (uint32_t hop = 0; hop < p_max_hops && frontier_count > 0; ++hop) {
        size_t next_count = 0;
        for (size_t f = 0; f < frontier_count; ++f) {
            const uint32_t cur = frontier[f];
            for (const auto &rel : synset_relations_) {
                if (rel.source_sememe_id == cur && rel.relation == WordNetRelationType::WN_HYPERNYM) {
                    if (rel.target_sememe_id == p_candidate_hypernym_id) {
                        return true;
                    }
                    if (next_count < next_frontier.size()) {
                        next_frontier[next_count++] = rel.target_sememe_id;
                    }
                } else if (rel.target_sememe_id == cur && rel.relation == WordNetRelationType::WN_HYPONYM) {
                    if (rel.source_sememe_id == p_candidate_hypernym_id) {
                        return true;
                    }
                    if (next_count < next_frontier.size()) {
                        next_frontier[next_count++] = rel.source_sememe_id;
                    }
                }
            }
        }
        frontier       = next_frontier;
        frontier_count = next_count;
    }
    return false;
}

uint32_t SemanticLexiconCore::register_semantic_frame(
    std::string_view p_frame_name,
    const SemanticFramePOD &p_frame
) {
    std::string norm = normalize_ascii_token(p_frame_name);
    const uint32_t h = hash_linguistic_token(norm);
    for (size_t i = 0; i < semantic_frames_.size() && i < frame_names_.size(); ++i) {
        if (semantic_frames_[i].frame_name_hash == h && frame_names_[i] == norm) {
            SemanticFramePOD copy = p_frame;
            copy.frame_id         = static_cast<uint32_t>(i);
            copy.frame_name_hash  = h;
            semantic_frames_[i]   = copy;
            return copy.frame_id;
        }
    }
    SemanticFramePOD copy = p_frame;
    copy.frame_id         = static_cast<uint32_t>(semantic_frames_.size());
    copy.frame_name_hash  = h;
    semantic_frames_.push_back(copy);
    frame_names_.push_back(std::move(norm));
    return copy.frame_id;
}

int32_t SemanticLexiconCore::find_semantic_frame_by_name(std::string_view p_frame_name) const noexcept {
    std::string norm = normalize_ascii_token(p_frame_name);
    if (norm.empty()) return -1;
    for (size_t i = 0; i < semantic_frames_.size() && i < frame_names_.size(); ++i) {
        if (frame_names_[i] == norm) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

float SemanticLexiconCore::evaluate_semantic_frame_role_fit(
    uint32_t p_frame_id,
    const SememeDescriptorPOD &p_agent_desc,
    const SememeDescriptorPOD &p_patient_desc,
    const SememeDescriptorPOD &p_instrument_desc
) const noexcept {
    if (p_frame_id >= semantic_frames_.size()) {
        return 0.0f;
    }
    const auto &frame = semantic_frames_[p_frame_id];
    auto fold64_to_32 = [](uint64_t w) noexcept -> uint32_t {
        return static_cast<uint32_t>(w & 0xFFFFFFFFULL) | static_cast<uint32_t>(w >> 32u);
    };
    const uint32_t agent_low32   = fold64_to_32(p_agent_desc.nuclear_seme_words[0]);
    const uint32_t patient_low32 = fold64_to_32(p_patient_desc.nuclear_seme_words[0]);
    const uint32_t inst_low32    = fold64_to_32(p_instrument_desc.nuclear_seme_words[0]);

    float score  = 0.0f;
    int checks   = 0;

    if (frame.required_agent_seme_mask32 != 0u) {
        ++checks;
        if ((agent_low32 & frame.required_agent_seme_mask32) != 0u) {
            score += 1.0f;
        }
    }
    if (frame.required_patient_seme_mask32 != 0u) {
        ++checks;
        if ((patient_low32 & frame.required_patient_seme_mask32) != 0u) {
            score += 1.0f;
        }
    }
    if (frame.required_instrument_mask32 != 0u && inst_low32 != 0u) {
        ++checks;
        if ((inst_low32 & frame.required_instrument_mask32) != 0u) {
            score += 1.0f;
        }
    }

    return (checks > 0) ? (score / static_cast<float>(checks)) : 1.0f;
}

// ============================================================================
// Matryoshka (MRL) Float-to-Binary Quantizer & LSH SimHash
// ============================================================================

void SemanticLexiconCore::quantize_mrl_float_embedding(
    std::span<const float> p_mrl_floats,
    SememeDescriptorPOD &r_target_descriptor,
    bool p_mean_center
) noexcept {
    if (p_mrl_floats.empty()) {
        return;
    }

    float threshold = 0.0f;
    if (p_mean_center) {
        double sum = 0.0;
        const size_t n = std::min<size_t>(p_mrl_floats.size(), 320);
        for (size_t i = 0; i < n; ++i) {
            sum += static_cast<double>(p_mrl_floats[i]);
        }
        threshold = static_cast<float>(sum / static_cast<double>(n));
    }

    // Leading 128 Matryoshka dimensions -> nuclear_seme_words[1..2]
    uint64_t mrl_w0 = 0ULL;
    uint64_t mrl_w1 = 0ULL;
    for (size_t i = 0; i < 64 && i < p_mrl_floats.size(); ++i) {
        if (p_mrl_floats[i] > threshold) {
            mrl_w0 |= (1ULL << i);
        }
    }
    for (size_t i = 64; i < 128 && i < p_mrl_floats.size(); ++i) {
        if (p_mrl_floats[i] > threshold) {
            mrl_w1 |= (1ULL << (i - 64));
        }
    }
    r_target_descriptor.set_mrl_binary_embedding_128(mrl_w0, mrl_w1);

    // Remaining dimensions (128..319) -> peripheral_seme_words[0..2]
    for (size_t i = 128; i < 320 && i < p_mrl_floats.size(); ++i) {
        if (p_mrl_floats[i] > threshold) {
            r_target_descriptor.set_peripheral_bit(static_cast<uint32_t>(i - 128));
        }
    }
}

std::array<uint64_t, 2> SemanticLexiconCore::compute_character_ngram_simhash128(
    std::string_view p_text
) noexcept {
    std::array<int32_t, 128> accum{};
    accum.fill(0);

    std::string padded = "^";
    padded.append(normalize_ascii_token(p_text));
    padded.push_back('$');

    // Accumulate signed hyperplane projections over character 2-grams and 3-grams
    for (size_t len = 2; len <= 3; ++len) {
        if (padded.size() < len) continue;
        for (size_t i = 0; i + len <= padded.size(); ++i) {
            const uint32_t ng_hash = hash_linguistic_token(std::string_view(padded.data() + i, len));
            SeedStreamPOD rng(static_cast<uint64_t>(ng_hash) * 0x9E3779B97F4A7C15ULL);
            const uint64_t h0 = rng.next_u64();
            const uint64_t h1 = rng.next_u64();
            for (size_t b = 0; b < 64; ++b) {
                accum[b]      += ((h0 & (1ULL << b)) != 0ULL) ? 1 : -1;
                accum[64 + b] += ((h1 & (1ULL << b)) != 0ULL) ? 1 : -1;
            }
        }
    }

    std::array<uint64_t, 2> out{0ULL, 0ULL};
    for (size_t b = 0; b < 64; ++b) {
        if (accum[b] >= 0)      out[0] |= (1ULL << b);
        if (accum[64 + b] >= 0) out[1] |= (1ULL << b);
    }
    return out;
}

// ============================================================================
// CMUdict ARPAbet Importer & Context-Sensitive G2P
// ============================================================================

bool SemanticLexiconCore::import_cmudict_entry(
    std::string_view p_word,
    std::string_view p_arpabet_pronunciation,
    UniversalPOS p_default_pos
) {
    // Strip CMUdict variant suffix like "(1)" or "(2)" from "READ(1)"
    size_t paren_pos = p_word.find('(');
    std::string_view clean_word = (paren_pos != std::string_view::npos) ? p_word.substr(0, paren_pos) : p_word;
    std::string norm_word = normalize_ascii_token(clean_word);
    if (norm_word.empty()) {
        return false;
    }

    std::vector<uint16_t> ph_masks;
    size_t pos = 0;
    while (pos < p_arpabet_pronunciation.size()) {
        while (pos < p_arpabet_pronunciation.size() &&
               (p_arpabet_pronunciation[pos] == ' ' || p_arpabet_pronunciation[pos] == '\t')) {
            ++pos;
        }
        if (pos >= p_arpabet_pronunciation.size()) break;
        size_t end = pos;
        while (end < p_arpabet_pronunciation.size() &&
               p_arpabet_pronunciation[end] != ' ' &&
               p_arpabet_pronunciation[end] != '\t' &&
               p_arpabet_pronunciation[end] != '\r' &&
               p_arpabet_pronunciation[end] != '\n') {
            ++end;
        }
        const uint16_t mask = arpabet_to_articulatory_mask(p_arpabet_pronunciation.substr(pos, end - pos));
        if (mask != FEAT_NONE) {
            ph_masks.push_back(mask);
        }
        pos = end;
    }

    if (ph_masks.empty()) {
        return false;
    }

    int32_t sem_id = find_sememe_by_label(norm_word);
    if (sem_id < 0) {
        SememeDescriptorPOD desc{};
        desc.taxeme_category = static_cast<uint8_t>(p_default_pos);
        desc.set_nuclear_bit(resolve_seme_bit(norm_word));
        sem_id = static_cast<int32_t>(register_sememe(norm_word, desc));
    }

    register_lexeme(norm_word, static_cast<uint32_t>(sem_id), p_default_pos, 0, 128, 1.0f, 0xFFFFFFFFu, ph_masks);
    return true;
}

uint32_t SemanticLexiconCore::import_cmudict_lines(std::string_view p_multiline_cmudict_text) {
    uint32_t imported = 0;
    size_t line_start = 0;
    while (line_start < p_multiline_cmudict_text.size()) {
        size_t line_end = p_multiline_cmudict_text.find('\n', line_start);
        if (line_end == std::string_view::npos) {
            line_end = p_multiline_cmudict_text.size();
        }
        std::string_view line = p_multiline_cmudict_text.substr(line_start, line_end - line_start);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        // Skip CMUdict comment lines starting with ";;;" or "#"
        if (!line.empty() && !line.starts_with(";;;") && !line.starts_with("#")) {
            size_t sep = line.find_first_of(" \t");
            if (sep != std::string_view::npos) {
                std::string_view word_part = line.substr(0, sep);
                std::string_view pron_part = line.substr(sep + 1);
                if (import_cmudict_entry(word_part, pron_part)) {
                    ++imported;
                }
            }
        }
        line_start = line_end + 1;
    }
    return imported;
}

void SemanticLexiconCore::approximate_phonemes_from_text(
    std::string_view p_text,
    std::vector<uint16_t> &r_out_masks
) const {
    r_out_masks.clear();
    std::string norm = normalize_ascii_token(p_text);
    if (norm.empty()) {
        return;
    }

    // 1. Built-in Irregular English Pronunciation Dictionary (CMUdict ARPAbet exceptions)
    struct IrregularG2PEntry {
        std::string_view word;
        std::string_view arpabet;
    };
    static constexpr std::array<IrregularG2PEntry, 24> IRREGULAR_G2P = {{
        {"knight",    "N AY1 T"},
        {"night",     "N AY1 T"},
        {"light",     "L AY1 T"},
        {"fight",     "F AY1 T"},
        {"sight",     "S AY1 T"},
        {"might",     "M AY1 T"},
        {"right",     "R AY1 T"},
        {"castle",    "K AE1 S AH0 L"},
        {"sword",     "S AO1 R D"},
        {"island",    "AY1 L AH0 N D"},
        {"ghost",     "G OW1 S T"},
        {"rogue",     "R OW1 G"},
        {"plague",    "P L EY1 G"},
        {"tongue",    "T AH1 NG"},
        {"guard",     "G AA1 R D"},
        {"guide",     "G AY1 D"},
        {"guilt",     "G IH1 L T"},
        {"rough",     "R AH1 F"},
        {"tough",     "T AH1 F"},
        {"enough",    "IH0 N AH1 F"},
        {"though",    "DH OW1"},
        {"through",   "TH R UW1"},
        {"thought",   "TH AO1 T"},
        {"knowledge", "N AA1 L AH0 JH"}
    }};

    for (const auto &irr : IRREGULAR_G2P) {
        if (norm == irr.word) {
            size_t pos = 0;
            while (pos < irr.arpabet.size()) {
                size_t end = irr.arpabet.find(' ', pos);
                if (end == std::string_view::npos) end = irr.arpabet.size();
                const uint16_t m = arpabet_to_articulatory_mask(irr.arpabet.substr(pos, end - pos));
                if (m != FEAT_NONE) r_out_masks.push_back(m);
                pos = end + 1;
            }
            return;
        }
    }

    // 2. Context-Sensitive Grapheme-to-Phoneme (G2P) Rule Cascade
    r_out_masks.reserve(norm.size());
    for (size_t i = 0; i < norm.size(); ++i) {
        const char c    = norm[i];
        const char next = (i + 1 < norm.size()) ? norm[i + 1] : '\0';
        const char next2= (i + 2 < norm.size()) ? norm[i + 2] : '\0';

        // Word-initial silent consonant clusters: 'kn', 'wr', 'gn', 'ps'
        if (i == 0 && ((c == 'k' && next == 'n') ||
                       (c == 'w' && next == 'r') ||
                       (c == 'g' && next == 'n') ||
                       (c == 'p' && next == 's'))) {
            continue;
        }

        // Trigraphs: 'igh' -> /AY/, 'tch' -> /CH/, 'dge' -> /JH/
        if (c == 'i' && next == 'g' && next2 == 'h') {
            r_out_masks.push_back(arpabet_to_articulatory_mask("AY"));
            i += 2;
            continue;
        }
        if (c == 't' && next == 'c' && next2 == 'h') {
            r_out_masks.push_back(arpabet_to_articulatory_mask("CH"));
            i += 2;
            continue;
        }
        if (c == 'd' && next == 'g' && (next2 == 'e' || next2 == 'i')) {
            r_out_masks.push_back(arpabet_to_articulatory_mask("JH"));
            i += 1;
            continue;
        }

        // Suffix '-tion' / '-sion' -> /SH AH N/
        if ((c == 't' || c == 's') && next == 'i' && next2 == 'o' &&
            (i + 3 < norm.size() && norm[i + 3] == 'n')) {
            r_out_masks.push_back(arpabet_to_articulatory_mask("SH"));
            r_out_masks.push_back(arpabet_to_articulatory_mask("AH"));
            r_out_masks.push_back(arpabet_to_articulatory_mask("N"));
            i += 3;
            continue;
        }

        // Digraphs: 'sh', 'ch', 'th', 'ng', 'ph', 'qu', 'wh', 'oo', 'ee', 'ea', 'ou', 'ow'
        if (c == 's' && next == 'h') { r_out_masks.push_back(arpabet_to_articulatory_mask("SH")); ++i; continue; }
        if (c == 'c' && next == 'h') { r_out_masks.push_back(arpabet_to_articulatory_mask("CH")); ++i; continue; }
        if (c == 't' && next == 'h') { r_out_masks.push_back(arpabet_to_articulatory_mask("TH")); ++i; continue; }
        if (c == 'n' && next == 'g') { r_out_masks.push_back(arpabet_to_articulatory_mask("NG")); ++i; continue; }
        if (c == 'p' && next == 'h') { r_out_masks.push_back(arpabet_to_articulatory_mask("F"));  ++i; continue; }
        if (c == 'w' && next == 'h') { r_out_masks.push_back(arpabet_to_articulatory_mask("W"));  ++i; continue; }
        if (c == 'c' && next == 'k') { r_out_masks.push_back(arpabet_to_articulatory_mask("K"));  ++i; continue; }
        if (c == 'o' && next == 'o') { r_out_masks.push_back(arpabet_to_articulatory_mask("UW")); ++i; continue; }
        if (c == 'e' && (next == 'e' || next == 'a')) { r_out_masks.push_back(arpabet_to_articulatory_mask("IY")); ++i; continue; }
        if (c == 'o' && (next == 'u' || next == 'w')) { r_out_masks.push_back(arpabet_to_articulatory_mask("AW")); ++i; continue; }

        // Skip geminate double letters ('ll', 'tt', 'ss', 'ff', 'pp', 'mm', 'nn')
        if (i > 0 && c == norm[i - 1]) {
            continue;
        }

        // Word-final silent 'e' (after at least 2 prior phonemes)
        if (c == 'e' && i + 1 == norm.size() && r_out_masks.size() >= 2) {
            continue;
        }

        // Context-sensitive soft 'c' (/s/ before e, i, y; else /k/) and soft 'g' (/dʒ/ before e, i, y; else /g/)
        if (c == 'c') {
            if (next == 'e' || next == 'i' || next == 'y') {
                r_out_masks.push_back(arpabet_to_articulatory_mask("S"));
            } else {
                r_out_masks.push_back(arpabet_to_articulatory_mask("K"));
            }
            continue;
        }
        if (c == 'g') {
            if (next == 'e' || next == 'i' || next == 'y') {
                r_out_masks.push_back(arpabet_to_articulatory_mask("JH"));
            } else {
                r_out_masks.push_back(arpabet_to_articulatory_mask("G"));
            }
            continue;
        }

        uint16_t mask = FEAT_NONE;
        switch (c) {
            case 'a': mask = arpabet_to_articulatory_mask("AE"); break;
            case 'e': mask = arpabet_to_articulatory_mask("EH"); break;
            case 'i':
            case 'y': mask = arpabet_to_articulatory_mask("IH"); break;
            case 'o': mask = arpabet_to_articulatory_mask("AA"); break;
            case 'u': mask = arpabet_to_articulatory_mask("AH"); break;
            case 'p': mask = arpabet_to_articulatory_mask("P");  break;
            case 'b': mask = arpabet_to_articulatory_mask("B");  break;
            case 't': mask = arpabet_to_articulatory_mask("T");  break;
            case 'd': mask = arpabet_to_articulatory_mask("D");  break;
            case 'k':
            case 'q': mask = arpabet_to_articulatory_mask("K");  break;
            case 'f': mask = arpabet_to_articulatory_mask("F");  break;
            case 'v': mask = arpabet_to_articulatory_mask("V");  break;
            case 's': mask = arpabet_to_articulatory_mask("S");  break;
            case 'z': mask = arpabet_to_articulatory_mask("Z");  break;
            case 'h': mask = arpabet_to_articulatory_mask("HH"); break;
            case 'm': mask = arpabet_to_articulatory_mask("M");  break;
            case 'n': mask = arpabet_to_articulatory_mask("N");  break;
            case 'l': mask = arpabet_to_articulatory_mask("L");  break;
            case 'r': mask = arpabet_to_articulatory_mask("R");  break;
            case 'w': mask = arpabet_to_articulatory_mask("W");  break;
            case 'j': mask = arpabet_to_articulatory_mask("JH"); break;
            case 'x':
                r_out_masks.push_back(arpabet_to_articulatory_mask("K"));
                mask = arpabet_to_articulatory_mask("S");
                break;
            default: break;
        }
        if (mask != FEAT_NONE) {
            r_out_masks.push_back(mask);
        }
    }
}

// ============================================================================
// SentencePiece / Hugging Face BPE Subword Tokenizer
// ============================================================================

uint32_t SemanticLexiconCore::register_bpe_vocab_entry(
    std::string_view p_piece,
    uint32_t p_token_id,
    float p_unigram_score,
    uint32_t p_mapped_sememe_id
) {
    const uint32_t h = hash_linguistic_token(p_piece);
    for (size_t i = 0; i < bpe_vocab_.size() && i < bpe_pieces_.size(); ++i) {
        if (bpe_vocab_[i].piece_hash == h && bpe_pieces_[i] == p_piece) {
            bpe_vocab_[i].bpe_token_id     = p_token_id;
            bpe_vocab_[i].unigram_score    = p_unigram_score;
            bpe_vocab_[i].mapped_sememe_id = p_mapped_sememe_id;
            return static_cast<uint32_t>(i);
        }
    }
    BPEVocabEntryPOD entry{};
    entry.bpe_token_id     = p_token_id;
    entry.piece_hash       = h;
    entry.unigram_score    = p_unigram_score;
    entry.mapped_sememe_id = p_mapped_sememe_id;
    bpe_vocab_.push_back(entry);
    bpe_pieces_.emplace_back(p_piece);
    return static_cast<uint32_t>(bpe_vocab_.size() - 1);
}

uint32_t SemanticLexiconCore::register_bpe_merge_rule(
    uint32_t p_left_token_id,
    uint32_t p_right_token_id,
    uint32_t p_merged_token_id,
    uint32_t p_priority_rank
) {
    BPEMergeRulePOD rule{};
    rule.left_token_id   = p_left_token_id;
    rule.right_token_id  = p_right_token_id;
    rule.merged_token_id = p_merged_token_id;
    rule.priority_rank   = p_priority_rank;
    bpe_merges_.push_back(rule);
    return static_cast<uint32_t>(bpe_merges_.size() - 1);
}

int32_t SemanticLexiconCore::find_bpe_token_by_piece(std::string_view p_piece) const noexcept {
    const uint32_t h = hash_linguistic_token(p_piece);
    for (size_t i = 0; i < bpe_vocab_.size() && i < bpe_pieces_.size(); ++i) {
        if (bpe_vocab_[i].piece_hash == h && bpe_pieces_[i] == p_piece) {
            return static_cast<int32_t>(bpe_vocab_[i].bpe_token_id);
        }
    }
    return -1;
}

void SemanticLexiconCore::tokenize_bpe_subwords(
    std::string_view p_text,
    std::vector<uint32_t> &r_out_bpe_ids,
    std::vector<std::string> *r_out_pieces
) const {
    r_out_bpe_ids.clear();
    if (r_out_pieces) {
        r_out_pieces->clear();
    }
    std::string norm = normalize_ascii_token(p_text);
    if (norm.empty()) {
        return;
    }

    // Split on '_' (normalized word boundaries) and run WordPiece / SentencePiece greedy longest-match
    // followed by BPE merge rules if registered
    size_t pos = 0;
    while (pos < norm.size()) {
        while (pos < norm.size() && norm[pos] == '_') ++pos;
        if (pos >= norm.size()) break;
        size_t word_end = norm.find('_', pos);
        if (word_end == std::string::npos) word_end = norm.size();
        std::string_view word = std::string_view(norm).substr(pos, word_end - pos);

        // Try BPE vocab table first if populated; otherwise fall back to Lexeme + Morpheme subword tables
        size_t cur = 0;
        while (cur < word.size()) {
            size_t best_len = 0;
            uint32_t best_id = 0xFFFFFFFFu;
            std::string best_piece;

            for (size_t len = word.size() - cur; len >= 1; --len) {
                std::string_view sub = word.substr(cur, len);
                if (!bpe_vocab_.empty()) {
                    const int32_t tid = find_bpe_token_by_piece(sub);
                    if (tid >= 0) {
                        best_len   = len;
                        best_id    = static_cast<uint32_t>(tid);
                        best_piece = std::string(sub);
                        break;
                    }
                    // Also check WordPiece "##" continuation prefix
                    if (cur > 0) {
                        std::string wp = "##";
                        wp.append(sub);
                        const int32_t wp_tid = find_bpe_token_by_piece(wp);
                        if (wp_tid >= 0) {
                            best_len   = len;
                            best_id    = static_cast<uint32_t>(wp_tid);
                            best_piece = std::move(wp);
                            break;
                        }
                    }
                }

                // Fallback to Lexeme / Morpheme tables
                const int32_t lx_idx = find_lexeme_by_surface(sub, 0);
                if (lx_idx >= 0) {
                    best_len   = len;
                    best_id    = (lexemes_[static_cast<size_t>(lx_idx)].slm_bpe_token_id != 0xFFFFFFFFu)
                                     ? lexemes_[static_cast<size_t>(lx_idx)].slm_bpe_token_id
                                     : static_cast<uint32_t>(lx_idx);
                    best_piece = std::string(sub);
                    break;
                }
                for (size_t m = 0; m < morphemes_.size() && m < morpheme_forms_.size(); ++m) {
                    if (morpheme_forms_[m] == sub) {
                        best_len   = len;
                        best_id    = 0x80000000u | static_cast<uint32_t>(m);
                        best_piece = (cur > 0) ? ("##" + std::string(sub)) : std::string(sub);
                        break;
                    }
                }
                if (best_len > 0) break;
            }

            if (best_len == 0) {
                // Single-character byte fallback (SentencePiece byte fallback `<0xXX>`)
                best_len   = 1;
                best_id    = static_cast<uint8_t>(word[cur]);
                best_piece = std::string(word.substr(cur, 1));
            }

            r_out_bpe_ids.push_back(best_id);
            if (r_out_pieces) {
                r_out_pieces->push_back(std::move(best_piece));
            }
            cur += best_len;
        }

        pos = word_end + 1;
    }

    // Apply iterative BPE merge rules if `bpe_merges_` is populated
    if (!bpe_merges_.empty() && r_out_bpe_ids.size() >= 2) {
        bool merged_any = true;
        while (merged_any && r_out_bpe_ids.size() >= 2) {
            merged_any = false;
            uint32_t best_rank = std::numeric_limits<uint32_t>::max();
            size_t best_pair_idx = 0;
            uint32_t replacement_id = 0;

            for (size_t i = 0; i + 1 < r_out_bpe_ids.size(); ++i) {
                for (const auto &rule : bpe_merges_) {
                    if (rule.left_token_id == r_out_bpe_ids[i] &&
                        rule.right_token_id == r_out_bpe_ids[i + 1] &&
                        rule.priority_rank < best_rank) {
                        best_rank      = rule.priority_rank;
                        best_pair_idx  = i;
                        replacement_id = rule.merged_token_id;
                    }
                }
            }
            if (best_rank != std::numeric_limits<uint32_t>::max()) {
                r_out_bpe_ids[best_pair_idx] = replacement_id;
                r_out_bpe_ids.erase(r_out_bpe_ids.begin() + static_cast<std::ptrdiff_t>(best_pair_idx + 1));
                merged_any = true;
            }
        }
    }
}

std::string SemanticLexiconCore::decode_bpe_tokens(std::span<const uint32_t> p_bpe_ids) const {
    std::string out;
    for (uint32_t id : p_bpe_ids) {
        bool found = false;
        for (size_t i = 0; i < bpe_vocab_.size() && i < bpe_pieces_.size(); ++i) {
            if (bpe_vocab_[i].bpe_token_id == id) {
                std::string_view piece = bpe_pieces_[i];
                if (piece.starts_with("##")) {
                    out.append(piece.substr(2));
                } else {
                    if (!out.empty()) out.push_back(' ');
                    out.append(piece);
                }
                found = true;
                break;
            }
        }
        if (!found) {
            for (size_t i = 0; i < lexemes_.size() && i < lexeme_surfaces_.size(); ++i) {
                if (lexemes_[i].slm_bpe_token_id == id || i == id) {
                    if (!out.empty()) out.push_back(' ');
                    out.append(lexeme_surfaces_[i]);
                    break;
                }
            }
        }
    }
    return out;
}

MorphologicalDecompositionResultPOD SemanticLexiconCore::decompose_word_morphemes(
    std::string_view p_word
) const {
    MorphologicalDecompositionResultPOD result{};
    std::string norm = normalize_ascii_token(p_word);
    if (norm.empty()) {
        return result;
    }

    // 1. Check exact Lexeme match first
    const int32_t direct_lx = find_lexeme_by_surface(norm, 0);
    if (direct_lx >= 0) {
        const auto &lx            = lexemes_[static_cast<size_t>(direct_lx)];
        result.matched_lexeme_id  = lx.lexeme_id;
        result.matched_sememe_id  = lx.sememe_id;
        result.stem_hash          = hash_linguistic_token(norm);
        result.inferred_category  = lx.category;
        if (lx.sememe_id < sememes_.size()) {
            result.composed_descriptor = sememes_[lx.sememe_id];
        }
        return result;
    }

    // 2. Check exact Sememe label match
    const int32_t direct_sem = find_sememe_by_label(norm);
    if (direct_sem >= 0) {
        const auto &sem           = sememes_[static_cast<size_t>(direct_sem)];
        result.matched_sememe_id  = sem.sememe_id;
        result.stem_hash          = sem.canonical_name_hash;
        result.inferred_category  = static_cast<UniversalPOS>(sem.taxeme_category);
        result.composed_descriptor = sem;
        return result;
    }

    // 3. Strip longest matching BOUND_PREFIX (if remaining stem >= 3 chars)
    std::string_view stem_view(norm);
    for (size_t i = 0; i < morphemes_.size() && i < morpheme_forms_.size(); ++i) {
        const auto &m = morphemes_[i];
        if (m.kind != MorphemeKind::BOUND_PREFIX) {
            continue;
        }
        const std::string &pref = morpheme_forms_[i];
        if (!pref.empty() &&
            stem_view.size() >= pref.size() + 3 &&
            stem_view.substr(0, pref.size()) == pref) {
            result.prefix_morpheme_id = m.morpheme_id;
            result.is_negated         = (m.negates_root != 0);
            if (m.primary_seme_id < SEME_BIT_CAPACITY) {
                result.composed_descriptor.set_peripheral_bit(m.primary_seme_id);
            }
            result.composed_descriptor.peripheral_seme_words[0] |= m.seme_modifier_mask;
            stem_view.remove_prefix(pref.size());
            break;
        }
    }

    // 4. Strip longest matching BOUND_SUFFIX or INFLECTIONAL_AFFIX (if remaining stem >= 3 chars)
    for (size_t i = 0; i < morphemes_.size() && i < morpheme_forms_.size(); ++i) {
        const auto &m = morphemes_[i];
        if (m.kind != MorphemeKind::BOUND_SUFFIX && m.kind != MorphemeKind::INFLECTIONAL_AFFIX) {
            continue;
        }
        const std::string &suff = morpheme_forms_[i];
        if (!suff.empty() &&
            stem_view.size() >= suff.size() + 3 &&
            stem_view.substr(stem_view.size() - suff.size()) == suff) {
            result.suffix_morpheme_id = m.morpheme_id;
            if (m.negates_root != 0) {
                result.is_negated = !result.is_negated;
            }
            if (m.category_shift != 0) {
                result.inferred_category = static_cast<UniversalPOS>(m.category_shift);
            }
            if (m.primary_seme_id < SEME_BIT_CAPACITY) {
                result.composed_descriptor.set_peripheral_bit(m.primary_seme_id);
            }
            result.composed_descriptor.peripheral_seme_words[0] |= m.seme_modifier_mask;
            stem_view.remove_suffix(suff.size());
            break;
        }
    }

    result.stem_hash = hash_linguistic_token(stem_view);

    // 5. Try matching the stripped stem against Lexemes / Sememes (including silent-'e' restoration)
    auto try_match_stem = [&](std::string_view p_candidate) -> bool {
        const int32_t lx_idx = find_lexeme_by_surface(p_candidate, 0);
        if (lx_idx >= 0) {
            const auto &lx           = lexemes_[static_cast<size_t>(lx_idx)];
            result.matched_lexeme_id = lx.lexeme_id;
            result.matched_sememe_id = lx.sememe_id;
            if (result.inferred_category == UniversalPOS::UNDEFINED) {
                result.inferred_category = lx.category;
            }
            if (lx.sememe_id < sememes_.size()) {
                if (result.is_negated) {
                    result.composed_descriptor.peripheral_seme_words[0] |=
                        sememes_[lx.sememe_id].nuclear_seme_words[0];
                    result.composed_descriptor.set_nuclear_bit(SEME_BIT_AFFECT_NEGATIVE);
                } else {
                    result.composed_descriptor.union_with(sememes_[lx.sememe_id]);
                }
            }
            return true;
        }
        const int32_t sem_idx = find_sememe_by_label(p_candidate);
        if (sem_idx >= 0) {
            const auto &sem          = sememes_[static_cast<size_t>(sem_idx)];
            result.matched_sememe_id = sem.sememe_id;
            if (result.inferred_category == UniversalPOS::UNDEFINED) {
                result.inferred_category = static_cast<UniversalPOS>(sem.taxeme_category);
            }
            if (!result.is_negated) {
                result.composed_descriptor.union_with(sem);
            } else {
                result.composed_descriptor.set_nuclear_bit(SEME_BIT_AFFECT_NEGATIVE);
            }
            return true;
        }
        return false;
    };

    if (!try_match_stem(stem_view)) {
        std::string with_e(stem_view);
        with_e.push_back('e');
        if (!try_match_stem(with_e)) {
            const uint32_t bit = resolve_seme_bit(stem_view);
            if (!result.is_negated) {
                result.composed_descriptor.set_nuclear_bit(bit);
            } else {
                result.composed_descriptor.set_peripheral_bit(bit);
                result.composed_descriptor.set_nuclear_bit(SEME_BIT_AFFECT_NEGATIVE);
            }
            // Also populate 128-bit character n-gram LSH SimHash for OOV word similarity
            const auto simhash = compute_character_ngram_simhash128(stem_view);
            result.composed_descriptor.set_mrl_binary_embedding_128(simhash[0], simhash[1]);
        }
    }

    return result;
}

// ============================================================================
// Componential & Binary-Quantized MRL Bitvector Algebra
// ============================================================================

float SemanticLexiconCore::compute_bitarray_jaccard(
    const std::array<uint64_t, 3> &p_a,
    const std::array<uint64_t, 3> &p_b
) noexcept {
    uint32_t inter_count = 0;
    uint32_t union_count = 0;
    for (size_t i = 0; i < 3; ++i) {
        inter_count += static_cast<uint32_t>(std::popcount(p_a[i] & p_b[i]));
        union_count += static_cast<uint32_t>(std::popcount(p_a[i] | p_b[i]));
    }
    if (union_count == 0u) {
        return 0.0f;
    }
    return static_cast<float>(inter_count) / static_cast<float>(union_count);
}

float SemanticLexiconCore::compute_bitarray_tversky(
    const std::array<uint64_t, 3> &p_prototype,
    const std::array<uint64_t, 3> &p_variant,
    float p_alpha,
    float p_beta
) noexcept {
    uint32_t inter_count   = 0;
    uint32_t proto_only    = 0;
    uint32_t variant_only  = 0;
    for (size_t i = 0; i < 3; ++i) {
        inter_count  += static_cast<uint32_t>(std::popcount(p_prototype[i] & p_variant[i]));
        proto_only   += static_cast<uint32_t>(std::popcount(p_prototype[i] & ~p_variant[i]));
        variant_only += static_cast<uint32_t>(std::popcount(~p_prototype[i] & p_variant[i]));
    }
    const float denom = static_cast<float>(inter_count) +
                        p_alpha * static_cast<float>(proto_only) +
                        p_beta * static_cast<float>(variant_only);
    if (denom <= 1e-6f) {
        return 0.0f;
    }
    return static_cast<float>(inter_count) / denom;
}

float SemanticLexiconCore::compute_bitarray_cosine(
    const std::array<uint64_t, 3> &p_a,
    const std::array<uint64_t, 3> &p_b
) noexcept {
    uint32_t inter_count = 0;
    uint32_t count_a     = 0;
    uint32_t count_b     = 0;
    for (size_t i = 0; i < 3; ++i) {
        inter_count += static_cast<uint32_t>(std::popcount(p_a[i] & p_b[i]));
        count_a     += static_cast<uint32_t>(std::popcount(p_a[i]));
        count_b     += static_cast<uint32_t>(std::popcount(p_b[i]));
    }
    if (count_a == 0u || count_b == 0u) {
        return 0.0f;
    }
    return static_cast<float>(inter_count) /
           std::sqrt(static_cast<float>(count_a) * static_cast<float>(count_b));
}

float SemanticLexiconCore::compute_descriptor_affinity(
    const SememeDescriptorPOD &p_a,
    const SememeDescriptorPOD &p_b,
    float p_nuclear_weight,
    float p_peripheral_weight
) noexcept {
    // 1. Symbolic Ontology Jaccard on Word 0 (bits 0..63)
    const uint64_t sym_inter = static_cast<uint64_t>(std::popcount(p_a.nuclear_seme_words[0] & p_b.nuclear_seme_words[0]));
    const uint64_t sym_union = static_cast<uint64_t>(std::popcount(p_a.nuclear_seme_words[0] | p_b.nuclear_seme_words[0]));
    const float sym_jaccard  = (sym_union > 0ULL) ? (static_cast<float>(sym_inter) / static_cast<float>(sym_union)) : 0.0f;

    // 2. Binary-Quantized MRL / SimHash similarity on Words 1..2 (bits 64..191)
    float nuc_score = sym_jaccard;
    if (p_a.has_mrl_binary_embedding() && p_b.has_mrl_binary_embedding()) {
        // Check if words[1..2] are dense binary embeddings (>= 24 bits set) vs sparse symbolic bits
        const uint32_t mrl_pop_a = static_cast<uint32_t>(
            std::popcount(p_a.nuclear_seme_words[1]) + std::popcount(p_a.nuclear_seme_words[2])
        );
        const uint32_t mrl_pop_b = static_cast<uint32_t>(
            std::popcount(p_b.nuclear_seme_words[1]) + std::popcount(p_b.nuclear_seme_words[2])
        );
        if (mrl_pop_a >= 16u && mrl_pop_b >= 16u) {
            const float mrl_sim = p_a.mrl_hamming_similarity_128(p_b);
            nuc_score = (sym_union > 0ULL) ? (0.70f * sym_jaccard + 0.30f * mrl_sim) : mrl_sim;
        } else {
            nuc_score = compute_bitarray_jaccard(p_a.nuclear_seme_words, p_b.nuclear_seme_words);
        }
    } else {
        nuc_score = compute_bitarray_jaccard(p_a.nuclear_seme_words, p_b.nuclear_seme_words);
    }

    const float per_jaccard = compute_bitarray_jaccard(p_a.peripheral_seme_words, p_b.peripheral_seme_words);

    // Cross-overlap bonus when a nuclear symbolic trait in A appears as peripheral in B (or vice versa)
    const uint32_t cross_inter = static_cast<uint32_t>(
        std::popcount(p_a.nuclear_seme_words[0] & p_b.peripheral_seme_words[0]) +
        std::popcount(p_a.peripheral_seme_words[0] & p_b.nuclear_seme_words[0])
    );
    const uint32_t cross_union = static_cast<uint32_t>(
        std::popcount(p_a.nuclear_seme_words[0] | p_b.nuclear_seme_words[0] |
                      p_a.peripheral_seme_words[0] | p_b.peripheral_seme_words[0])
    );
    const float cross_bonus = (cross_union > 0u)
                                  ? (0.15f * static_cast<float>(cross_inter) / static_cast<float>(cross_union))
                                  : 0.0f;

    const bool has_per_a = p_a.peripheral_popcount() > 0u;
    const bool has_per_b = p_b.peripheral_popcount() > 0u;
    const float eff_nuc_w = (has_per_a && has_per_b) ? p_nuclear_weight : 1.0f;
    const float eff_per_w = (has_per_a && has_per_b) ? p_peripheral_weight : 0.0f;
    const float weight_sum = std::max(0.0001f, eff_nuc_w + eff_per_w);

    const float raw = ((eff_nuc_w * nuc_score) + (eff_per_w * per_jaccard)) / weight_sum + cross_bonus;
    return std::clamp(raw, 0.0f, 1.0f);
}

float SemanticLexiconCore::compute_descriptor_tversky_affinity(
    const SememeDescriptorPOD &p_query_or_proto,
    const SememeDescriptorPOD &p_candidate,
    float p_alpha,
    float p_beta,
    float p_nuclear_weight,
    float p_peripheral_weight
) noexcept {
    // Evaluate Tversky on the symbolic ontology word[0] first so dense MRL SimHash bits
    // never dilute exact symbolic feature contrast
    std::array<uint64_t, 3> sym_q{p_query_or_proto.nuclear_seme_words[0], 0ULL, 0ULL};
    std::array<uint64_t, 3> sym_c{p_candidate.nuclear_seme_words[0], 0ULL, 0ULL};
    float nuc_tv = compute_bitarray_tversky(sym_q, sym_c, p_alpha, p_beta);
    if (sym_q[0] == 0ULL || sym_c[0] == 0ULL) {
        nuc_tv = compute_bitarray_tversky(
            p_query_or_proto.nuclear_seme_words,
            p_candidate.nuclear_seme_words,
            p_alpha,
            p_beta
        );
    }

    const float per_tv = compute_bitarray_tversky(
        p_query_or_proto.peripheral_seme_words,
        p_candidate.peripheral_seme_words,
        p_alpha,
        p_beta
    );
    const bool has_per_q = (p_query_or_proto.peripheral_popcount() > 0u) && (p_candidate.peripheral_popcount() > 0u);
    const float eff_nuc_w = has_per_q ? p_nuclear_weight : 1.0f;
    const float eff_per_w = has_per_q ? p_peripheral_weight : 0.0f;
    const float weight_sum = std::max(0.0001f, eff_nuc_w + eff_per_w);
    return std::clamp(((eff_nuc_w * nuc_tv) + (eff_per_w * per_tv)) / weight_sum, 0.0f, 1.0f);
}

float SemanticLexiconCore::compute_sememe_affinity(
    uint32_t p_sememe_a,
    uint32_t p_sememe_b
) const noexcept {
    if (p_sememe_a >= sememes_.size() || p_sememe_b >= sememes_.size()) {
        return 0.0f;
    }
    if (p_sememe_a == p_sememe_b) {
        return 1.0f;
    }
    float base_aff = compute_descriptor_affinity(sememes_[p_sememe_a], sememes_[p_sememe_b]);
    if (is_hypernym_of(p_sememe_a, p_sememe_b, 4) || is_hypernym_of(p_sememe_b, p_sememe_a, 4)) {
        base_aff = std::max(base_aff, 0.75f);
    }
    return base_aff;
}

// ============================================================================
// Needleman-Wunsch / Levenshtein-Hamming Phonetic Sequence Alignment
// ============================================================================

float SemanticLexiconCore::compute_phoneme_slice_distance(
    std::span<const uint16_t> p_slice_a,
    std::span<const uint16_t> p_slice_b
) noexcept {
    if (p_slice_a.empty() && p_slice_b.empty()) {
        return 0.0f;
    }
    static constexpr size_t MAX_DP_LEN = 32;
    const size_t n = std::min(p_slice_a.size(), MAX_DP_LEN);
    const size_t m = std::min(p_slice_b.size(), MAX_DP_LEN);
    static constexpr float GAP_PENALTY = 3.5f;

    if (n == 0) return static_cast<float>(m) * GAP_PENALTY;
    if (m == 0) return static_cast<float>(n) * GAP_PENALTY;

    // Stack-allocated 2-row DP buffer (zero heap allocations!)
    std::array<float, MAX_DP_LEN + 1> prev_row{};
    std::array<float, MAX_DP_LEN + 1> curr_row{};

    for (size_t j = 0; j <= m; ++j) {
        prev_row[j] = static_cast<float>(j) * GAP_PENALTY;
    }

    for (size_t i = 1; i <= n; ++i) {
        curr_row[0] = static_cast<float>(i) * GAP_PENALTY;
        for (size_t j = 1; j <= m; ++j) {
            const float sub_cost = weighted_phoneme_substitution_cost(p_slice_a[i - 1], p_slice_b[j - 1]);
            const float match_or_sub = prev_row[j - 1] + sub_cost;
            const float del_cost     = prev_row[j] + GAP_PENALTY;
            const float ins_cost     = curr_row[j - 1] + GAP_PENALTY;
            curr_row[j] = std::min({match_or_sub, del_cost, ins_cost});
        }
        prev_row = curr_row;
    }

    const size_t max_len = std::max(n, m);
    return prev_row[m] / static_cast<float>(max_len);
}

uint32_t SemanticLexiconCore::find_best_matching_lexeme_from_noisy_phonemes(
    std::span<const uint16_t> p_noisy_phoneme_masks,
    uint32_t p_expected_sememe_id,
    uint16_t p_preferred_lect_id
) const noexcept {
    if (p_noisy_phoneme_masks.empty() || lexemes_.empty()) {
        return 0xFFFFFFFFu;
    }

    float best_cost  = std::numeric_limits<float>::max();
    uint32_t best_id = 0xFFFFFFFFu;

    for (size_t i = 0; i < lexemes_.size(); ++i) {
        const auto &lx = lexemes_[i];
        std::span<const uint16_t> cand_slice = get_lexeme_phoneme_masks(static_cast<uint32_t>(i));
        if (cand_slice.empty()) {
            continue;
        }

        const float phonetic_dist = compute_phoneme_slice_distance(p_noisy_phoneme_masks, cand_slice);

        float context_bonus = 0.0f;
        if (p_expected_sememe_id < sememes_.size() && lx.sememe_id < sememes_.size()) {
            context_bonus = 1.25f * compute_sememe_affinity(lx.sememe_id, p_expected_sememe_id);
        }

        const float lect_bonus = (lx.lect_id == p_preferred_lect_id) ? 0.35f : 0.0f;
        const float freq_bonus = 0.15f * std::clamp(lx.corpus_frequency, 0.0f, 2.0f);

        const float total_cost = phonetic_dist - context_bonus - lect_bonus - freq_bonus;
        if (total_cost < best_cost) {
            best_cost = total_cost;
            best_id   = lx.lexeme_id;
        }
    }

    return best_id;
}

// ============================================================================
// Neuro-Symbolic SLM Vocabulary Masking & GBNF Export
// ============================================================================

void SemanticLexiconCore::build_slm_vocabulary_mask(
    const SememeDescriptorPOD &p_context_descriptor,
    uint16_t p_lect_id,
    uint16_t p_allowed_category_mask,
    float p_min_affinity,
    std::vector<uint32_t> &r_allowed_bpe_tokens,
    std::vector<uint32_t> &r_allowed_lexeme_ids
) const {
    r_allowed_bpe_tokens.clear();
    r_allowed_lexeme_ids.clear();

    const bool filter_by_semes = (p_context_descriptor.symbolic_ontology_word() != 0ULL ||
                                  p_context_descriptor.peripheral_popcount() > 0u);

    for (const auto &lx : lexemes_) {
        if (p_lect_id != 0u && lx.lect_id != 0u && lx.lect_id != p_lect_id) {
            continue;
        }
        const uint16_t cat_bit = static_cast<uint16_t>(1u << static_cast<uint8_t>(lx.category));
        if (p_allowed_category_mask != 0u && (p_allowed_category_mask & cat_bit) == 0u) {
            continue;
        }
        if (filter_by_semes && lx.sememe_id < sememes_.size()) {
            const float aff = compute_descriptor_tversky_affinity(
                p_context_descriptor,
                sememes_[lx.sememe_id],
                0.35f,
                0.65f
            );
            if (aff < p_min_affinity) {
                continue;
            }
        }
        r_allowed_lexeme_ids.push_back(lx.lexeme_id);
        if (lx.slm_bpe_token_id != 0xFFFFFFFFu) {
            r_allowed_bpe_tokens.push_back(lx.slm_bpe_token_id);
        }
    }
}

std::string SemanticLexiconCore::export_gbnf_grammar_for_constraints(
    const SememeDescriptorPOD &p_context_descriptor,
    uint16_t p_lect_id,
    uint16_t p_allowed_category_mask,
    float p_min_affinity
) const {
    std::vector<uint32_t> bpe_ids;
    std::vector<uint32_t> lex_ids;
    build_slm_vocabulary_mask(
        p_context_descriptor,
        p_lect_id,
        p_allowed_category_mask,
        p_min_affinity,
        bpe_ids,
        lex_ids
    );

    std::vector<std::string_view> nouns;
    std::vector<std::string_view> verbs;
    std::vector<std::string_view> adjs;
    std::vector<std::string_view> others;

    for (uint32_t lx_id : lex_ids) {
        if (lx_id >= lexemes_.size() || lx_id >= lexeme_surfaces_.size()) continue;
        std::string_view surf = lexeme_surfaces_[lx_id];
        if (surf.empty()) continue;
        switch (lexemes_[lx_id].category) {
            case UniversalPOS::NOUN:
            case UniversalPOS::PROPN:     nouns.push_back(surf); break;
            case UniversalPOS::VERB:
            case UniversalPOS::AUX:       verbs.push_back(surf); break;
            case UniversalPOS::ADJECTIVE: adjs.push_back(surf);  break;
            default:                      others.push_back(surf);break;
        }
    }

    auto format_rule = [](std::string_view p_rule_name, const std::vector<std::string_view> &p_items, std::string_view p_fallback) {
        std::string line(p_rule_name);
        line += " ::= ";
        if (p_items.empty()) {
            line += "\"";
            line += p_fallback;
            line += "\"\n";
            return line;
        }
        for (size_t i = 0; i < p_items.size(); ++i) {
            if (i > 0) line += " | ";
            line += "\"";
            line += p_items[i];
            line += "\"";
        }
        line += "\n";
        return line;
    };

    std::string gbnf;
    gbnf.reserve(512);
    gbnf += "# Auto-compiled IdeamLinguistics GBNF Grammar (llama.cpp / XGrammar / llguidance)\n";
    gbnf += "root ::= noun-phrase (\" \" verb-phrase)?\n";
    gbnf += "noun-phrase ::= (adj \" \")? noun\n";
    gbnf += "verb-phrase ::= verb (\" \" noun-phrase)?\n";
    gbnf += format_rule("noun", nouns, "entity");
    gbnf += format_rule("verb", verbs, "interact");
    gbnf += format_rule("adj",  adjs,  "active");
    if (!others.empty()) {
        gbnf += format_rule("modifier", others, "");
    }
    return gbnf;
}

} // namespace ideam::core
