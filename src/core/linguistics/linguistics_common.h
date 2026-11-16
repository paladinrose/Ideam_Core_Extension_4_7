#pragma once

/**
 * linguistics_common.h
 * Pure C++26 industry-standard NLP & DOD primitives for `IdeamLinguistics` (`src/core/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & INDUSTRY-STANDARD NLP ALIGNMENT (`IdeamLinguistics`)
 * ============================================================================
 * 1. HYBRID SYMBOLIC + BINARY-QUANTIZED MATRYOSHKA (MRL) SEMANTICS (`SememeDescriptorPOD`, 64B):
 *    - Unites deterministic symbolic ontology (Pottier/Greimas componential analysis, WordNet
 *      synsets, SUMO upper ontology) with modern neural **Matryoshka Representation Learning (MRL)**
 *      and **1-Bit Binary Quantization**:
 *      * `nuclear_seme_words[0]` (64 bits): Exact symbolic & structural invariants (`CanonicalSemeBit`
 *        `0..63`). Locked during `IdeamQuantumDarwinism` Pointer-State stabilization and matched
 *        deterministically by CSS selectors (`IdeamGames`, `IdeamProceduralGeneration`, `IdeamWFC`).
 *      * `nuclear_seme_words[1..2]` (128 bits): Binary-quantized MRL-128 core embedding (sign-binarized
 *        leading dimensions of a Matryoshka text embedding or character n-gram LSH SimHash).
 *      * `peripheral_seme_words[0..2]` (192 bits): Contextual/connotative semes + extended MRL-320
 *        binary embedding tail bits that can relax along Quantum Darwinism bridge paths.
 *      * All similarity queries (`Jaccard`, `Tversky`, and `MRL Hamming Cosine Proxy`) execute in
 *        branchless SIMD `std::popcount(a ^ b)` and `std::popcount(a & b)` in 1–3 CPU cycles!
 *
 * 2. UNIVERSAL DEPENDENCIES (UD v2 `UniversalPOS` & `UDDependencyRelation`):
 *    - Conforms to the international **Universal Dependencies v2 (UD)** standard (`UPOS` 17 tags
 *      and `DepRel` syntactic relations) used by spaCy, Stanza, UDPipe, and CoNLL-U treebanks,
 *      while keeping `LexicalCategory` as a backwards-compatible alias.
 *
 * 3. FRAMENET / PROPBANK SEMANTIC FRAMES & WORDNET SYNSET RELATIONS (`SemanticFramePOD`, 32B):
 *    - Models predicate-argument structure (`FrameID`, evoking `predicate_sememe_id`, and core
 *      thematic roles: `AGENT`/`ARG0`, `PATIENT`/`ARG1`, `INSTRUMENT`/`ARG2`, `LOCATION`) plus
 *      32-bit selectional restriction masks and WordNet taxonomic links (`HYPERNYM`, `HYPONYM`,
 *      `HOLONYM`, `MERONYM`, `ANTONYM`, `ENTAILMENT`).
 *
 * 4. PANPHON / IPA ARTICULATORY FEATURES & CMUDICT ARPABET G2P (`PhonemePOD`, 16B):
 *    - Aligns the 16-bit `ArticulatoryFeatureBits` vector with **PanPhon** (Mortensen et al.) and
 *      Jakobson-Chomsky-Halle feature geometry, supporting both Unicode IPA and standard **CMUdict
 *      ARPAbet** (`AA1`, `AE0`, `CH`, `SH`, `TH`, etc.) for seamless integration with external
 *      TTS, ASR, and forced-alignment tools.
 *
 * 5. SENTENCEPIECE / HUGGING FACE BPE SUBWORD TOKENIZATION & GBNF / XGRAMMAR DECODING:
 *    - Native C++26 POD structures (`BPEVocabEntryPOD`, `BPEMergeRulePOD`) for importing standard
 *      Hugging Face `tokenizer.json` and SentencePiece `.vocab` files, plus **GBNF (GGML BNF)**
 *      and **XGrammar** pushdown token mask compilation for zero-hallucination local SLMs.
 */

#include "../procedural_generation/procgen_common.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

static constexpr uint32_t SEME_BIT_CAPACITY            = 192u; // 3 x 64-bit words per bank
static constexpr uint32_t SYMBOLIC_SEME_BIT_CAPACITY   = 64u;  // Word 0 of nuclear_seme_words
static constexpr uint32_t MRL_NUCLEAR_BIT_CAPACITY     = 128u; // Words 1..2 of nuclear_seme_words
static constexpr uint32_t MRL_PERIPHERAL_BIT_CAPACITY  = 192u; // Words 0..2 of peripheral_seme_words

/**
 * 16-bit PanPhon / Jakobson-Chomsky-Halle articulatory feature bitmask for `PhonemePOD`.
 * Ordered from most noise-resilient (Tier 1: Syllabic/Sonorant/Voiced) to most noise-fragile
 * under broadband masking (Tier 4: Place of Articulation).
 */
enum ArticulatoryFeatureBits : uint16_t {
    FEAT_NONE        = 0u,
    // --- Tier 1: Prosodic & Major Class Features (Highest Noise Resilience) ---
    FEAT_SYLLABIC    = 1u << 0,  //PanPhon `syl`: Vowel or syllabic consonant (+syllabic)
    FEAT_SONORANT    = 1u << 1,  //PanPhon `son`: Vowels, glides, liquids, nasals (+sonorant)
    FEAT_VOICED      = 1u << 2,  //PanPhon `voi`: Vocal cord vibration (+voice: /b/, /d/, /g/, /v/, /z/)
    // --- Tier 2: Vowel Space Features (High-Medium Noise Resilience) ---
    FEAT_VOWEL_HIGH  = 1u << 3,  //PanPhon `hi`:  High tongue body (/i/, /u/, /ɪ/, /ʊ/)
    FEAT_VOWEL_LOW   = 1u << 4,  //PanPhon `lo`:  Low tongue body (/ɑ/, /æ/, /ɔ/)
    FEAT_VOWEL_BACK  = 1u << 5,  //PanPhon `back`:Back tongue body (/u/, /o/, /ɑ/)
    FEAT_VOWEL_ROUND = 1u << 6,  //PanPhon `rnd`: Rounded lips (/u/, /o/, /ɔ/, /w/)
    // --- Tier 3: Manner of Articulation Features (Medium Noise Resilience) ---
    FEAT_CONTINUANT  = 1u << 7,  //PanPhon `cont`:Fricatives/approximants (+cont) vs Stops (-cont)
    FEAT_NASAL       = 1u << 8,  //PanPhon `nas`: Nasal resonance (+nasal: /m/, /n/, /ŋ/)
    FEAT_STRIDENT    = 1u << 9,  //PanPhon `strid`:High-frequency turbulence (+strident: /s/, /z/, /ʃ/)
    FEAT_LATERAL     = 1u << 10, //PanPhon `lat`: Lateral airflow (+lateral: /l/)
    // --- Tier 4: Place of Articulation Features (Most Fragile in Noisy Environments) ---
    FEAT_LABIAL      = 1u << 11, //PanPhon `lab`: Bilabial / labiodental (/p/, /b/, /m/, /f/, /v/)
    FEAT_CORONAL     = 1u << 12, //PanPhon `cor`: Dental / alveolar / postalveolar (/t/, /d/, /s/, /n/)
    FEAT_ANTERIOR    = 1u << 13, //PanPhon `ant`: Front of alveolar ridge (+ant) vs postalveolar (-ant)
    FEAT_DORSAL      = 1u << 14, //PanPhon `dor`: Palatal / velar / uvular (/k/, /g/, /ŋ/, /j/)
    FEAT_GLOTTAL     = 1u << 15, //PanPhon `sg`:  Laryngeal / spread-glottis / glottal (/h/, /ʔ/)

    // --- Tier Masks for Noisy-Channel Degradation & Perceptual Distance ---
    FEAT_TIER1_PROSODIC_MASK = FEAT_SYLLABIC | FEAT_SONORANT | FEAT_VOICED,
    FEAT_TIER2_VOWEL_MASK    = FEAT_VOWEL_HIGH | FEAT_VOWEL_LOW | FEAT_VOWEL_BACK | FEAT_VOWEL_ROUND,
    FEAT_TIER3_MANNER_MASK   = FEAT_CONTINUANT | FEAT_NASAL | FEAT_STRIDENT | FEAT_LATERAL,
    FEAT_TIER4_PLACE_MASK    = FEAT_LABIAL | FEAT_CORONAL | FEAT_ANTERIOR | FEAT_DORSAL | FEAT_GLOTTAL
};

enum class MorphemeKind : uint8_t {
    FREE_ROOT          = 0,
    BOUND_PREFIX       = 1,
    BOUND_SUFFIX       = 2,
    INFLECTIONAL_AFFIX = 3,
    CLITIC             = 4,
    BPE_SUBWORD        = 5  // SentencePiece / Hugging Face BPE subword unit
};

/**
 * Universal Dependencies v2 (UD) 17 Universal Part-of-Speech (`UPOS`) tags.
 * Numeric values 0..9 preserve exact backwards compatibility with `LexicalCategory`.
 */
enum class UniversalPOS : uint8_t {
    UNDEFINED   = 0,  // UD `X`: Other / unclassified
    NOUN        = 1,  // UD `NOUN`: Common noun
    VERB        = 2,  // UD `VERB`: Main verb / predicate
    ADJECTIVE   = 3,  // UD `ADJ`: Adjective / modifier
    ADJ         = 3,
    ADVERB      = 4,  // UD `ADV`: Adverb
    ADV         = 4,
    ADPOSITION  = 5,  // UD `ADP`: Preposition / postposition
    ADP         = 5,
    PRONOUN     = 6,  // UD `PRON`: Pronoun
    PRON        = 6,
    DETERMINER  = 7,  // UD `DET`: Determiner / article
    DET         = 7,
    CONJUNCTION = 8,  // UD `CCONJ`: Coordinating conjunction
    CCONJ       = 8,
    PARTICLE    = 9,  // UD `PART`: Particle
    PART        = 9,
    PROPN       = 10, // UD `PROPN`: Proper noun (named entity, character, place)
    NUM         = 11, // UD `NUM`: Numeral / quantity
    AUX         = 12, // UD `AUX`: Auxiliary / modal verb
    SCONJ       = 13, // UD `SCONJ`: Subordinating conjunction
    INTJ        = 14, // UD `INTJ`: Interjection / exclamation
    SYM         = 15, // UD `SYM`: Symbol / operator
    PUNCT       = 16  // UD `PUNCT`: Punctuation
};

// Backwards-compatible alias for existing callers across Ideam plugins
using LexicalCategory = UniversalPOS;

/**
 * Universal Dependencies v2 (`DepRel`) core syntactic dependency relations.
 */
enum class UDDependencyRelation : uint8_t {
    DEP_NONE     = 0,
    DEP_ROOT     = 1,  // `root`: Root predicate or head noun of phrase/identifier
    DEP_NSUBJ    = 2,  // `nsubj`: Nominal subject (FrameNet Agent / PropBank ARG0)
    DEP_OBJ      = 3,  // `obj`: Direct object (FrameNet Patient/Theme / PropBank ARG1)
    DEP_IOBJ     = 4,  // `iobj`: Indirect object (Recipient / Goal / PropBank ARG2)
    DEP_OBL      = 5,  // `obl`: Oblique nominal (Instrument, Location, Temporal)
    DEP_AMOD     = 6,  // `amod`: Adjectival modifier ("rusty sword" -> rusty -> sword)
    DEP_NMOD     = 7,  // `nmod`: Nominal modifier ("castle tower" -> castle -> tower)
    DEP_COMPOUND = 8,  // `compound`: Multi-word / CamelCase compound modifier
    DEP_ADVMOD   = 9,  // `advmod`: Adverbial modifier / intensifier / diminisher
    DEP_NEG      = 10, // `neg`: Negation modifier ("not", "without", "never", "un-")
    DEP_CASE     = 11, // `case`: Case-marking adposition ("in", "with", "from")
    DEP_CONJ     = 12, // `conj`: Conjunct
    DEP_AUX      = 13  // `aux`: Auxiliary
};

/**
 * WordNet / OntoWordNet / BabelNet standard lexical-semantic relation types.
 */
enum class WordNetRelationType : uint8_t {
    WN_HYPERNYM       = 0, // `is-a` generalization (e.g., `tower` -> `structure`)
    WN_HYPONYM        = 1, // `is-a` specialization (e.g., `structure` -> `tower`)
    WN_PART_HOLONYM   = 2, // `part-of` whole (e.g., `tower` -> `castle`)
    WN_PART_MERONYM   = 3, // `has-part` component (e.g., `castle` -> `tower`)
    WN_ANTONYM        = 4, // Semantic opposite (e.g., `locked` <-> `unlocked`)
    WN_ENTAILMENT     = 5, // Verb entailment (e.g., `snore` -> `sleep`)
    WN_CAUSE          = 6, // Causal relation (e.g., `ignite` -> `burn`)
    WN_SIMILAR_TO     = 7, // Adjective/satellite similarity
    WN_DERIVATIONALLY = 8, // Cross-POS morphological derivation
    WN_EVOKES_FRAME   = 9  // FrameNet Lexical Unit -> Semantic Frame link
};

/**
 * FrameNet / PropBank / VerbNet standard thematic participant roles (Frame Elements).
 */
enum class ThematicRole : uint8_t {
    ROLE_NONE       = 0,
    ROLE_AGENT      = 1, // PropBank `ARG0`: Intentional actor / instigator
    ROLE_PATIENT    = 2, // PropBank `ARG1`: Entity undergoing state change or affected
    ROLE_THEME      = 3, // PropBank `ARG1`: Entity moved, experienced, or described
    ROLE_RECIPIENT  = 4, // PropBank `ARG2`: Receiver or beneficiary
    ROLE_INSTRUMENT = 5, // PropBank `ARG2/MNR`: Tool, weapon, or medium used
    ROLE_LOCATION   = 6, // PropBank `ARGM-LOC`: Spatial setting or container
    ROLE_SOURCE     = 7, // PropBank `ARGM-DIR`: Starting point or origin
    ROLE_GOAL       = 8, // PropBank `ARGM-GOL`: Destination or target state
    ROLE_TIME       = 9, // PropBank `ARGM-TMP`: Temporal anchor or duration
    ROLE_MANNER     = 10,// PropBank `ARGM-MNR`: Speed, style, or intensity
    ROLE_CAUSE      = 11 // PropBank `ARGM-CAU`: Reason or trigger condition
};

/**
 * Semantic Domain IDs organizing the symbolic & ontological space (aligned with WordNet
 * Lexicographer Domains and Ideam engine domains).
 */
enum class SemanticDomain : uint16_t {
    GENERAL           = 0,
    SPATIAL_TOPOLOGY  = 1, // 2D, 3D, 4D, top_down, first_person, isometric, vr_xr, interior, exterior
    TEMPORAL_ERA      = 2, // prehistoric, ancient, medieval, renaissance, industrial, modern, scifi
    GENRE_AND_TONE    = 3, // fantasy, cyberpunk, steampunk, gothic, mystery, horror, comedy, tactical
    SYSTEM_MECHANIC   = 4, // rpg, action, turn_based, real_time, puzzle, platformer, simulation, procedural
    ENTITY_ONTOLOGY   = 5, // agent, character, creature, prop, item, weapon, garment, structure, room, aperture
    CAPABILITY_ACTION = 6, // movable, interactive, speakable, audible, destructible, lockable, container, combat
    AFFECT_AND_SOCIAL = 7, // benevolent, hostile, fearful, curious, sacred, profane, formal, colloquial, archaic
    UI_AND_REFLECTION = 8  // label_display, text_input, clickable_trigger, value_range, stateful_toggle, event_signal
};

/**
 * Bitflags recording which Godot or external NLP channels contributed to a `LinguisticProfilePOD`.
 */
enum LinguisticSourceChannelFlags : uint16_t {
    LING_SOURCE_NONE                 = 0u,
    LING_SOURCE_CLASS_HIERARCHY      = 1u << 0, // ClassDB class, parent classes, and Script global_name
    LING_SOURCE_OBJECT_AND_NODE_NAME = 1u << 1, // Node::get_name(), Resource::get_name(), path stem
    LING_SOURCE_NODE_GROUPS          = 1u << 2, // Node::get_groups()
    LING_SOURCE_UI_TEXT_CONTROLS     = 1u << 3, // Label, RichTextLabel, Button, LineEdit, TextEdit, Window, Tooltip
    LING_SOURCE_REFLECTED_PROPERTIES = 1u << 4, // Object::get_property_list() names, enum hints, ranges, values
    LING_SOURCE_REFLECTED_METHODS    = 1u << 5, // Object::get_method_list() action verbs & getter/setter predicates
    LING_SOURCE_REFLECTED_SIGNALS    = 1u << 6, // Object::get_signal_list() eventive/participial transitions
    LING_SOURCE_OBJECT_METADATA      = 1u << 7, // Object::get_meta_list() and MetadataLinguisticsManager tags
    LING_SOURCE_EXPLICIT_RESOURCE    = 1u << 8, // Attached SememeResource or SemanticTagSetResource
    LING_SOURCE_SCENE_STATE          = 1u << 9, // Zero-instantiation PackedScene / SceneState inspection
    LING_SOURCE_MRL_EMBEDDING        = 1u << 10,// Imported Matryoshka (MRL) binary-quantized embedding
    LING_SOURCE_WORDNET_FRAMENET     = 1u << 11,// Imported WordNet synset or FrameNet semantic frame
    LING_SOURCE_ALL                  = 0x0FFFu
};

/**
 * Canonical Symbolic Seme Bit Indices (`0..63` in `nuclear_seme_words[0]`).
 * Bits `64..191` (`nuclear_seme_words[1..2]`) store the 128-bit Binary-Quantized
 * Matryoshka (MRL) embedding or character n-gram LSH SimHash!
 */
enum CanonicalSemeBit : uint32_t {
    // --- Spatial & Dimensional (0..11) ---
    SEME_BIT_DIM_1D             = 0,
    SEME_BIT_DIM_2D             = 1,
    SEME_BIT_DIM_3D             = 2,
    SEME_BIT_DIM_4D             = 3,
    SEME_BIT_SPATIAL_TOP_DOWN   = 4,
    SEME_BIT_SPATIAL_FIRST_PERS = 5,
    SEME_BIT_SPATIAL_THIRD_PERS = 6,
    SEME_BIT_SPATIAL_ISOMETRIC  = 7,
    SEME_BIT_SPATIAL_SIDE_VIEW  = 8,
    SEME_BIT_SPATIAL_XR_VR      = 9,
    SEME_BIT_SPATIAL_INTERIOR   = 10,
    SEME_BIT_SPATIAL_EXTERIOR   = 11,

    // --- Era & World Setting (12..21) ---
    SEME_BIT_ERA_ANCIENT        = 12,
    SEME_BIT_ERA_MEDIEVAL       = 13,
    SEME_BIT_ERA_RENAISSANCE    = 14,
    SEME_BIT_ERA_INDUSTRIAL     = 15,
    SEME_BIT_ERA_MODERN         = 16,
    SEME_BIT_ERA_FUTURISTIC     = 17,
    SEME_BIT_GENRE_FANTASY      = 18,
    SEME_BIT_GENRE_SCIFI        = 19,
    SEME_BIT_GENRE_GOTHIC_HORROR= 20,
    SEME_BIT_GENRE_MYSTERY      = 21,

    // --- Gameplay & System Mechanics (22..29) ---
    SEME_BIT_MECHANIC_RPG       = 22,
    SEME_BIT_MECHANIC_ACTION    = 23,
    SEME_BIT_MECHANIC_STRATEGY  = 24,
    SEME_BIT_MECHANIC_TURN_BASED= 25,
    SEME_BIT_MECHANIC_REAL_TIME = 26,
    SEME_BIT_MECHANIC_PUZZLE    = 27,
    SEME_BIT_MECHANIC_SIMULATION= 28,
    SEME_BIT_MECHANIC_PROCEDURAL= 29,

    // --- Ontology & Structural Role (30..43) ---
    SEME_BIT_ONTOLOGY_AGENT     = 30,
    SEME_BIT_ONTOLOGY_CREATURE  = 31,
    SEME_BIT_ONTOLOGY_PROP_ITEM = 32,
    SEME_BIT_ONTOLOGY_WEAPON    = 33,
    SEME_BIT_ONTOLOGY_GARMENT   = 34,
    SEME_BIT_ONTOLOGY_STRUCTURE = 35,
    SEME_BIT_ONTOLOGY_ROOM_SPACE= 36,
    SEME_BIT_ONTOLOGY_APERTURE  = 37, // Door, gate, window, archway, portal
    SEME_BIT_ONTOLOGY_VERTICAL  = 38, // Stairs, ladder, elevator, tower, shaft
    SEME_BIT_ONTOLOGY_CONTAINER = 39,
    SEME_BIT_ONTOLOGY_HAZARD    = 40,
    SEME_BIT_ONTOLOGY_LIGHT     = 41,
    SEME_BIT_ONTOLOGY_AUDIO     = 42,
    SEME_BIT_ONTOLOGY_SENSOR    = 43,

    // --- Capabilities, Affordances & States (44..55) ---
    SEME_BIT_CAP_MOVABLE        = 44,
    SEME_BIT_CAP_INTERACTIVE    = 45,
    SEME_BIT_CAP_COMMUNICATIVE  = 46,
    SEME_BIT_CAP_COMBAT         = 47,
    SEME_BIT_CAP_LOCKABLE       = 48,
    SEME_BIT_CAP_DESTRUCTIBLE   = 49,
    SEME_BIT_STATE_ACTIVE       = 50,
    SEME_BIT_STATE_LOCKED       = 51,
    SEME_BIT_STATE_DAMAGED      = 52,
    SEME_BIT_STATE_HIDDEN       = 53,
    SEME_BIT_AFFECT_POSITIVE    = 54,
    SEME_BIT_AFFECT_NEGATIVE    = 55,

    // --- Godot UI & Reflection Archetypes (56..63) ---
    SEME_BIT_UI_DISPLAY_TEXT    = 56, // Label, RichTextLabel, Label3D
    SEME_BIT_UI_TEXT_INPUT      = 57, // LineEdit, TextEdit, CodeEdit
    SEME_BIT_UI_ACTION_TRIGGER  = 58, // Button, CheckBox, OptionButton
    SEME_BIT_UI_VALUE_GAUGE     = 59, // Range, Slider, SpinBox, ProgressBar, CircularGauge
    SEME_BIT_REFL_HAS_METHODS   = 60,
    SEME_BIT_REFL_HAS_SIGNALS   = 61,
    SEME_BIT_REFL_HAS_METADATA  = 62,
    SEME_BIT_REFL_SCRIPTED      = 63,

    SEME_BIT_DYNAMIC_HASH_START = 64  // Bits 64..191 (128 bits): MRL Binary Embedding / SimHash
};

/**
 * Deterministic case-insensitive ASCII FNV-1a 32-bit hash for linguistic tokens,
 * seme tags, class names, and morpheme stems.
 */
[[nodiscard]] constexpr uint32_t hash_linguistic_token(std::string_view p_token) noexcept {
    uint32_t hash = 2166136261u;
    for (char c : p_token) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 'A' && uc <= 'Z') {
            uc = static_cast<unsigned char>(uc + ('a' - 'A'));
        }
        if (uc == '-' || uc == ' ') {
            uc = '_';
        }
        hash ^= static_cast<uint32_t>(uc);
        hash *= 16777619u;
    }
    return hash ? hash : 1u;
}

[[nodiscard]] constexpr uint32_t hash_to_dynamic_seme_bit(uint32_t p_hash) noexcept {
    return SEME_BIT_DYNAMIC_HASH_START + ((p_hash ^ (p_hash >> 16u)) & 127u);
}

/**
 * Converts a Universal Dependencies UPOS string ("NOUN", "VERB", "ADJ", "PROPN", etc.)
 * to `UniversalPOS`.
 */
[[nodiscard]] constexpr UniversalPOS parse_upos_tag(std::string_view p_upos) noexcept {
    const uint32_t h = hash_linguistic_token(p_upos);
    if (h == hash_linguistic_token("noun") || h == hash_linguistic_token("n"))       return UniversalPOS::NOUN;
    if (h == hash_linguistic_token("verb") || h == hash_linguistic_token("v"))       return UniversalPOS::VERB;
    if (h == hash_linguistic_token("adj")  || h == hash_linguistic_token("adjective") || h == hash_linguistic_token("a") || h == hash_linguistic_token("s")) return UniversalPOS::ADJECTIVE;
    if (h == hash_linguistic_token("adv")  || h == hash_linguistic_token("adverb")  || h == hash_linguistic_token("r")) return UniversalPOS::ADVERB;
    if (h == hash_linguistic_token("adp")  || h == hash_linguistic_token("adposition")) return UniversalPOS::ADPOSITION;
    if (h == hash_linguistic_token("pron") || h == hash_linguistic_token("pronoun")) return UniversalPOS::PRONOUN;
    if (h == hash_linguistic_token("det")  || h == hash_linguistic_token("determiner")) return UniversalPOS::DETERMINER;
    if (h == hash_linguistic_token("cconj")|| h == hash_linguistic_token("conjunction")) return UniversalPOS::CONJUNCTION;
    if (h == hash_linguistic_token("part") || h == hash_linguistic_token("particle")) return UniversalPOS::PARTICLE;
    if (h == hash_linguistic_token("propn")|| h == hash_linguistic_token("proper_noun")) return UniversalPOS::PROPN;
    if (h == hash_linguistic_token("num")  || h == hash_linguistic_token("numeral")) return UniversalPOS::NUM;
    if (h == hash_linguistic_token("aux")  || h == hash_linguistic_token("auxiliary")) return UniversalPOS::AUX;
    if (h == hash_linguistic_token("sconj")) return UniversalPOS::SCONJ;
    if (h == hash_linguistic_token("intj") || h == hash_linguistic_token("interjection")) return UniversalPOS::INTJ;
    if (h == hash_linguistic_token("sym")  || h == hash_linguistic_token("symbol"))  return UniversalPOS::SYM;
    if (h == hash_linguistic_token("punct")|| h == hash_linguistic_token("punctuation")) return UniversalPOS::PUNCT;
    return UniversalPOS::UNDEFINED;
}

[[nodiscard]] constexpr std::string_view upos_to_string(UniversalPOS p_pos) noexcept {
    switch (p_pos) {
        case UniversalPOS::NOUN:        return "NOUN";
        case UniversalPOS::VERB:        return "VERB";
        case UniversalPOS::ADJECTIVE:   return "ADJ";
        case UniversalPOS::ADVERB:      return "ADV";
        case UniversalPOS::ADPOSITION:  return "ADP";
        case UniversalPOS::PRONOUN:     return "PRON";
        case UniversalPOS::DETERMINER:  return "DET";
        case UniversalPOS::CONJUNCTION: return "CCONJ";
        case UniversalPOS::PARTICLE:    return "PART";
        case UniversalPOS::PROPN:       return "PROPN";
        case UniversalPOS::NUM:         return "NUM";
        case UniversalPOS::AUX:         return "AUX";
        case UniversalPOS::SCONJ:       return "SCONJ";
        case UniversalPOS::INTJ:        return "INTJ";
        case UniversalPOS::SYM:         return "SYM";
        case UniversalPOS::PUNCT:       return "PUNCT";
        default:                        return "X";
    }
}

/**
 * PhonemePOD (16 Bytes, alignas(16))
 * Compact DOD representation of a single phoneme, its 16-bit PanPhon articulatory feature
 * bitvector, and its packed 4-char CMUdict ARPAbet symbol (e.g., `"AE1"`, `"SH"`).
 */
struct alignas(16) PhonemePOD {
    uint32_t phoneme_id         = 0;
    uint32_t ipa_codepoint      = 0;   // Unicode IPA symbol (e.g. U+0070 'p', U+0283 'ʃ')
    uint16_t articulatory_mask  = 0;   // ArticulatoryFeatureBits (PanPhon 16-bit vector)
    uint16_t duration_ms        = 80;  // Typical phoneme duration for prosody timing
    uint8_t  sonority_rank      = 1;   // 1 (voiceless stop) .. 10 (low vowel) for syllable nuclei
    uint8_t  stress_level       = 0;   // 0 = unstressed, 1 = primary ('1'), 2 = secondary ('2')
    char     arpabet_code[2]    = {'\0', '\0'}; // 2-char CMUdict ARPAbet base symbol (e.g. 'S','H')

    [[nodiscard]] constexpr uint32_t feature_distance(uint16_t p_other_mask) const noexcept {
        return static_cast<uint32_t>(std::popcount(static_cast<uint16_t>(articulatory_mask ^ p_other_mask)));
    }
};
static_assert(sizeof(PhonemePOD) == 16, "PhonemePOD must be 16 bytes.");

/**
 * SemeDescriptorPOD (16 Bytes, alignas(16))
 * Atomic micro-component of meaning used in symbolic componential analysis.
 */
struct alignas(16) SemeDescriptorPOD {
    uint32_t seme_id            = 0;      // Bit index [0..191] in SememeDescriptorPOD
    uint32_t name_hash          = 0;      // FNV-1a hash of canonical seme tag
    uint16_t domain_id          = 0;      // SemanticDomain
    uint16_t dimension_id       = 0;      // Axis within domain
    uint16_t antonym_seme_id    = 0xFFFFu;// Opposing seme bit index (if any)
    uint8_t  is_nuclear_default = 1;
    uint8_t  reserved           = 0;
};
static_assert(sizeof(SemeDescriptorPOD) == 16, "SemeDescriptorPOD must be 16 bytes.");

/**
 * SememeDescriptorPOD (64 Bytes, alignas(64))
 * Hybrid Neuro-Symbolic Descriptor uniting:
 *   - `nuclear_seme_words[0]` (64 bits): Exact Symbolic Ontology & Structural Invariants.
 *   - `nuclear_seme_words[1..2]` (128 bits): Binary-Quantized Matryoshka (MRL-128) Core Embedding
 *     (or dynamic symbolic bits when operating in pure symbolic mode).
 *   - `peripheral_seme_words[0..2]` (192 bits): Connotative Semes + Extended MRL-320 Binary Tail.
 *   - `wordnet_synset_offset`: Standard 32-bit WordNet synset offset (or canonical_name_hash).
 */
struct alignas(64) SememeDescriptorPOD {
    std::array<uint64_t, 3> nuclear_seme_words{0ULL, 0ULL, 0ULL};    // 24B: [0]=64b Symbolic, [1..2]=128b MRL/Nuclear
    std::array<uint64_t, 3> peripheral_seme_words{0ULL, 0ULL, 0ULL}; // 24B: 192b Peripheral / MRL Tail
    uint32_t sememe_id          = 0;                                  // 4B
    uint32_t canonical_name_hash= 0;                                  // 4B: Or WordNet synset 8-digit offset
    float    emotive_valence    = 0.0f;                               // 4B: [-1.0 negative .. +1.0 positive]
    uint8_t  sememic_form       = 2;                                  // 1B: 2=PrimaryDenotation, 4..6=Connotative
    uint8_t  taxeme_category    = 0;                                  // 1B: UniversalPOS (UPOS)
    uint16_t primary_domain_id  = 0;                                  // 2B -> 64B total

    constexpr void clear_bits() noexcept {
        nuclear_seme_words    = {0ULL, 0ULL, 0ULL};
        peripheral_seme_words = {0ULL, 0ULL, 0ULL};
    }

    constexpr void set_nuclear_bit(uint32_t p_bit) noexcept {
        if (p_bit < SEME_BIT_CAPACITY) {
            nuclear_seme_words[p_bit >> 6u] |= (1ULL << (p_bit & 63u));
        }
    }

    constexpr void clear_nuclear_bit(uint32_t p_bit) noexcept {
        if (p_bit < SEME_BIT_CAPACITY) {
            nuclear_seme_words[p_bit >> 6u] &= ~(1ULL << (p_bit & 63u));
        }
    }

    [[nodiscard]] constexpr bool test_nuclear_bit(uint32_t p_bit) const noexcept {
        return (p_bit < SEME_BIT_CAPACITY) &&
               ((nuclear_seme_words[p_bit >> 6u] & (1ULL << (p_bit & 63u))) != 0ULL);
    }

    constexpr void set_peripheral_bit(uint32_t p_bit) noexcept {
        if (p_bit < SEME_BIT_CAPACITY) {
            peripheral_seme_words[p_bit >> 6u] |= (1ULL << (p_bit & 63u));
        }
    }

    constexpr void clear_peripheral_bit(uint32_t p_bit) noexcept {
        if (p_bit < SEME_BIT_CAPACITY) {
            peripheral_seme_words[p_bit >> 6u] &= ~(1ULL << (p_bit & 63u));
        }
    }

    [[nodiscard]] constexpr bool test_peripheral_bit(uint32_t p_bit) const noexcept {
        return (p_bit < SEME_BIT_CAPACITY) &&
               ((peripheral_seme_words[p_bit >> 6u] & (1ULL << (p_bit & 63u))) != 0ULL);
    }

    [[nodiscard]] constexpr bool test_any_bit(uint32_t p_bit) const noexcept {
        return test_nuclear_bit(p_bit) || test_peripheral_bit(p_bit);
    }

    [[nodiscard]] constexpr uint64_t symbolic_ontology_word() const noexcept {
        return nuclear_seme_words[0];
    }

    [[nodiscard]] constexpr bool has_mrl_binary_embedding() const noexcept {
        return (nuclear_seme_words[1] | nuclear_seme_words[2]) != 0ULL;
    }

    constexpr void set_mrl_binary_embedding_128(uint64_t p_word0, uint64_t p_word1) noexcept {
        nuclear_seme_words[1] = p_word0;
        nuclear_seme_words[2] = p_word1;
    }

    [[nodiscard]] constexpr uint32_t mrl_hamming_distance_128(const SememeDescriptorPOD &p_other) const noexcept {
        return static_cast<uint32_t>(
            std::popcount(nuclear_seme_words[1] ^ p_other.nuclear_seme_words[1]) +
            std::popcount(nuclear_seme_words[2] ^ p_other.nuclear_seme_words[2])
        );
    }

    [[nodiscard]] constexpr float mrl_hamming_similarity_128(const SememeDescriptorPOD &p_other) const noexcept {
        if (!has_mrl_binary_embedding() || !p_other.has_mrl_binary_embedding()) {
            return 0.0f;
        }
        const uint32_t dist = mrl_hamming_distance_128(p_other);
        return std::clamp(1.0f - (static_cast<float>(dist) / 128.0f), 0.0f, 1.0f);
    }

    [[nodiscard]] constexpr uint32_t nuclear_popcount() const noexcept {
        return static_cast<uint32_t>(
            std::popcount(nuclear_seme_words[0]) +
            std::popcount(nuclear_seme_words[1]) +
            std::popcount(nuclear_seme_words[2])
        );
    }

    [[nodiscard]] constexpr uint32_t peripheral_popcount() const noexcept {
        return static_cast<uint32_t>(
            std::popcount(peripheral_seme_words[0]) +
            std::popcount(peripheral_seme_words[1]) +
            std::popcount(peripheral_seme_words[2])
        );
    }

    constexpr void union_with(const SememeDescriptorPOD &p_other) noexcept {
        for (size_t i = 0; i < 3; ++i) {
            nuclear_seme_words[i]    |= p_other.nuclear_seme_words[i];
            peripheral_seme_words[i] |= p_other.peripheral_seme_words[i];
        }
    }

    [[nodiscard]] constexpr bool contains_all_nuclear(const SememeDescriptorPOD &p_required) const noexcept {
        // Check hard symbolic ontology word[0] first; if p_required also specifies bits in [1..2], verify them
        if ((nuclear_seme_words[0] & p_required.nuclear_seme_words[0]) != p_required.nuclear_seme_words[0]) {
            return false;
        }
        return true;
    }

    [[nodiscard]] constexpr uint32_t compute_coherence_trait_mask32() const noexcept {
        const uint64_t folded = nuclear_seme_words[0] ^
                                std::rotl(nuclear_seme_words[1], 21) ^
                                std::rotl(nuclear_seme_words[2], 42);
        return static_cast<uint32_t>((folded & 0xFFFFFFFFULL) ^ (folded >> 32u));
    }
};
static_assert(sizeof(SememeDescriptorPOD) == 64, "SememeDescriptorPOD must be 64 bytes.");

/**
 * SemanticFramePOD (32 Bytes, alignas(16))
 * FrameNet / PropBank / VerbNet predicate-argument structure for DOD execution across
 * `IdeamLinguistics`, `IdeamMinds` (`FactPod`), `IdeamNarratives` (`Incident`), and `IdeamGames`.
 */
struct alignas(16) SemanticFramePOD {
    uint32_t frame_id                      = 0;          // 4B
    uint32_t frame_name_hash               = 0;          // 4B: e.g. hash("Commerce_buy"), hash("Attack")
    uint32_t evoking_predicate_sememe_id   = 0xFFFFFFFFu;// 4B: Verb/Event sememe evoking this frame
    uint32_t required_agent_seme_mask32    = 0;          // 4B: Selectional restriction on ARG0 (Agent)
    uint32_t required_patient_seme_mask32  = 0;          // 4B: Selectional restriction on ARG1 (Patient/Theme)
    uint32_t required_instrument_mask32    = 0;          // 4B: Selectional restriction on ARG2 (Instrument)
    uint32_t consequent_state_seme_mask32  = 0;          // 4B: State semes inflicted on Patient (e.g. DAMAGED, LOCKED)
    uint16_t parent_frame_id               = 0xFFFFu;    // 2B: FrameNet inheritance (`Inherits_from`)
    uint8_t  core_role_mask                = 0x07u;      // 1B: Bitmask of required `ThematicRole`s
    uint8_t  valence_polarity              = 128;        // 1B: 0 = hostile/destructive .. 255 = constructive -> 32B
};
static_assert(sizeof(SemanticFramePOD) == 32, "SemanticFramePOD must be 32 bytes.");

/**
 * SynsetRelationPOD (16 Bytes, alignas(16))
 * Directed ontological edge in the WordNet / FrameNet graph (`source -> target`).
 */
struct alignas(16) SynsetRelationPOD {
    uint32_t source_sememe_id       = 0;
    uint32_t target_sememe_id       = 0;
    float    relation_weight        = 1.0f;
    WordNetRelationType relation    = WordNetRelationType::WN_HYPERNYM;
    uint8_t  reserved[3]            = {0};
};
static_assert(sizeof(SynsetRelationPOD) == 16, "SynsetRelationPOD must be 16 bytes.");

/**
 * BPEVocabEntryPOD (16 Bytes, alignas(16))
 * Subword token entry imported from Hugging Face `tokenizer.json` or SentencePiece `.vocab`.
 */
struct alignas(16) BPEVocabEntryPOD {
    uint32_t bpe_token_id           = 0;
    uint32_t piece_hash             = 0;
    uint32_t mapped_sememe_id       = 0xFFFFFFFFu;
    float    unigram_score          = 0.0f; // SentencePiece log-probability or BPE rank score
};
static_assert(sizeof(BPEVocabEntryPOD) == 16, "BPEVocabEntryPOD must be 16 bytes.");

/**
 * BPEMergeRulePOD (16 Bytes, alignas(16))
 * Byte-Pair Encoding merge rule `(left_token_id, right_token_id) -> merged_token_id` at `priority_rank`.
 */
struct alignas(16) BPEMergeRulePOD {
    uint32_t left_token_id          = 0;
    uint32_t right_token_id         = 0;
    uint32_t merged_token_id        = 0;
    uint32_t priority_rank          = 0;
};
static_assert(sizeof(BPEMergeRulePOD) == 16, "BPEMergeRulePOD must be 16 bytes.");

/**
 * MorphemePOD (32 Bytes, alignas(16))
 * Minimal morphological or BPE subword unit linking form (phoneme slice) to meaning (seme delta mask).
 */
struct alignas(16) MorphemePOD {
    uint32_t morpheme_id          = 0;
    uint32_t form_string_hash     = 0;
    uint32_t phoneme_csr_offset   = 0;
    uint16_t phoneme_count        = 0;
    uint16_t primary_seme_id      = 0;
    uint64_t seme_modifier_mask   = 0ULL;
    MorphemeKind kind             = MorphemeKind::FREE_ROOT;
    uint8_t  negates_root         = 0; // 1 if prefix/suffix negates nuclear semes (e.g., "un-", "-less")
    uint8_t  category_shift       = 0; // Optional UniversalPOS shift caused by derivational affix
    uint8_t  reserved[5]          = {0};
};
static_assert(sizeof(MorphemePOD) == 32, "MorphemePOD must be 32 bytes.");

/**
 * LexemePOD (32 Bytes, alignas(16))
 * Lexical entry binding a `SememeDescriptorPOD` to its `MorphemePOD` and `PhonemePOD` slices.
 */
struct alignas(16) LexemePOD {
    uint32_t lexeme_id            = 0;
    uint32_t sememe_id            = 0;
    uint32_t morpheme_csr_offset  = 0;
    uint16_t morpheme_count       = 0;
    uint16_t lect_id              = 0;   // Dialect / sociolect / register ID
    uint32_t phoneme_csr_offset   = 0;
    uint16_t phoneme_count        = 0;
    UniversalPOS category         = UniversalPOS::NOUN;
    uint8_t  register_formality   = 128; // 0 = slang/vulgar, 128 = neutral, 255 = high/archaic
    float    corpus_frequency     = 1.0f;
    uint32_t slm_bpe_token_id     = 0xFFFFFFFFu; // Mapping to HuggingFace / SentencePiece BPE vocab
};
static_assert(sizeof(LexemePOD) == 32, "LexemePOD must be 32 bytes.");

/**
 * LectDescriptorPOD (32 Bytes, alignas(16))
 * Dialect, sociolect, or situational register descriptor.
 */
struct alignas(16) LectDescriptorPOD {
    uint16_t lect_id                = 0;
    uint8_t  min_formality          = 0;
    uint8_t  max_formality          = 255;
    uint32_t lect_name_hash         = 0;
    uint64_t favored_peripheral_mask= 0ULL;
    uint16_t phoneme_shift_mask     = 0;    // Articulatory feature systematic shift (accent)
    uint16_t parent_lect_id         = 0;
    float    slm_logit_bias         = 1.0f;
    uint8_t  reserved[8]            = {0};
};
static_assert(sizeof(LectDescriptorPOD) == 32, "LectDescriptorPOD must be 32 bytes.");

/**
 * LinguisticProfilePOD (64 Bytes, alignas(64))
 * Cache-aligned 64-byte per-object semantic profile harvested by `LinguisticObjectProfiler`
 * and cached in Godot Object metadata (`MetadataLinguisticsManager`) or streamed directly
 * into `MemoryManagerDOD` (`LinguisticsBufferSlot::PRIMARY_DESCRIPTOR_POD`).
 *
 * Layout-identical in its first 48 bytes (`nuclear_seme_words` + `peripheral_seme_words`)
 * to `SememeDescriptorPOD` so `SemeComponentialAffinityQueryLogic` can execute over either
 * buffer without branching.
 */
struct alignas(64) LinguisticProfilePOD {
    std::array<uint64_t, 3> nuclear_seme_words{0ULL, 0ULL, 0ULL};    // 24B: [0]=64b Symbolic, [1..2]=128b MRL
    std::array<uint64_t, 3> peripheral_seme_words{0ULL, 0ULL, 0ULL}; // 24B: 192b Peripheral / MRL Tail
    uint32_t primary_sememe_id    = 0;                                // 4B
    uint32_t object_name_hash     = 0;                                // 4B: Hash of Node/Resource name or synset
    float    emotive_valence      = 0.0f;                             // 4B: Aggregate valence [-1.0 .. +1.0]
    uint16_t source_channel_mask  = LING_SOURCE_NONE;                 // 2B: LinguisticSourceChannelFlags
    uint8_t  dominant_category    = static_cast<uint8_t>(UniversalPOS::NOUN); // 1B: UPOS
    uint8_t  sememic_form         = 2;                                // 1B: SememicForm -> 64B total

    [[nodiscard]] constexpr SememeDescriptorPOD to_sememe_descriptor() const noexcept {
        SememeDescriptorPOD desc{};
        desc.nuclear_seme_words    = nuclear_seme_words;
        desc.peripheral_seme_words = peripheral_seme_words;
        desc.sememe_id             = primary_sememe_id;
        desc.canonical_name_hash   = object_name_hash;
        desc.emotive_valence       = emotive_valence;
        desc.sememic_form          = sememic_form;
        desc.taxeme_category       = dominant_category;
        return desc;
    }
};
static_assert(sizeof(LinguisticProfilePOD) == 64, "LinguisticProfilePOD must be 64 bytes.");

} // namespace ideam::core
