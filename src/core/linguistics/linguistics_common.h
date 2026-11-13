#pragma once

/**
 * linguistics_common.h
 * Pure C++26 structural semantics, phonology, morphology, lexicology, and neuro-symbolic
 * SLM constraint primitives for `IdeamLinguistics` (`src/core/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (`IdeamLinguistics`)
 * ============================================================================
 * 1. COMPONENTIAL SEMANTICS (`Seme` -> `Sememe`):
 *    - Grounded in structural componential analysis (Pottier, Greimas, Coseriu):
 *      * A `Seme` (`SemeDescriptorPOD`) is an indivisible micro-component of meaning
 *        (e.g., `[+SPATIAL_TOP_DOWN]`, `[+ERA_MEDIEVAL]`, `[+GENRE_FANTASY]`, `[+SYSTEM_RPG]`).
 *      * A `Sememe` (`SememeDescriptorPOD`) is a bundle of Semes partitioned into:
 *          - `nuclear_semes` (128-bit mask): Indispensable core identity traits without which
 *            an entity or concept loses its qualitative category.
 *          - `peripheral_semes` (128-bit mask): Contextual, stylistic, dialectal, or emotive
 *            connotative traits (`SememicForm`: denotation vs. emotive/evaluative connotation).
 *    - ENGINE-WIDE UNIFICATION:
 *      * Every asset, `GameplayResource`, `Narreme`, `ProceduralPattern`, `Style3DResource`,
 *        and `CharacterBlueprintResource` can be tagged with `Sememe` signatures.
 *      * Natural language prompts in Editor Wizards (e.g., `"top down, medieval, fantasy RPG"`)
 *        decompose into unioned `nuclear_semes` and `peripheral_semes` bitmasks that drive
 *        CSS-style selector cascades across `IdeamGames`, `IdeamProceduralGeneration`,
 *        `IdeamWaveFunctionCollapse`, `IdeamNarratives`, `IdeamDesign`, and `IdeamCharacters`.
 *      * In `IdeamQuantumDarwinism`, a token's `nuclear_semes` automatically define its
 *        immutable Pointer State traits, while its `peripheral_semes` define the mutable
 *        properties that can relax along pointer-to-conflict bridge paths!
 *
 * 2. PHONOLOGY & DISTINCTIVE FEATURE BITVECTORS (`PhonemePOD`):
 *    - Grounded in Jakobson-Chomsky-Halle Distinctive Feature Theory and Miller & Nicely (1955)
 *      noisy-channel speech confusion matrices:
 *      * Instead of storing opaque audio clips or raw characters, each `PhonemePOD` carries a
 *        16-bit `ArticulatoryFeatureBits` bitmask encoding:
 *          - Laryngeal / Voicing (`VOICED`, `ASPIRATED`) — highest resilience to channel noise.
 *          - Manner / Continuant (`SYLLABIC`, `SONORANT`, `CONTINUANT`, `NASAL`, `STRIDENT`, `LATERAL`) —
 *            medium resilience to noise.
 *          - Place of Articulation (`LABIAL`, `CORONAL`, `ANTERIOR`, `DORSAL`, `GLOTTAL`) —
 *            most fragile under background party chatter / broadband noise (`/p/` <-> `/t/` <-> `/k/`).
 *          - Vowel Space (`VOWEL_HIGH`, `VOWEL_LOW`, `VOWEL_BACK`, `VOWEL_ROUND`).
 *      * When `IdeamCommunications` degrades an utterance across a noisy `Medium`, it flips
 *        fragile Place/Manner bits according to Signal-to-Noise Ratio (SNR). The listener's
 *        decoder then matches the corrupted `uint16_t` feature stream against known `LexemePOD`
 *        phoneme slices via SIMD Hamming distance (`std::popcount(received ^ candidate)`),
 *        producing phonetically realistic mishearings ("mondegreens") rather than random
 *        character gibberish!
 *
 * 3. MORPHOLOGY, LEXICOLOGY & DIALECT LECTS (`MorphemePOD`, `LexemePOD`, `LectDescriptorPOD`):
 *    - `MorphemePOD`: Root, prefix, suffix, or inflectional bound morpheme carrying a delta
 *      `seme_mask` and a slice of `PhonemePOD`s.
 *    - `LexemePOD`: Pairs a `sememe_id` with its constituent `MorphemePOD` slice and surface
 *      `PhonemePOD` sequence, optionally scoped to a specific dialect/register `LectID`.
 *
 * 4. SMALL LANGUAGE MODEL (SLM) NEURO-SYMBOLIC OVERLAP:
 *    - While SLMs operate on Byte-Pair Encoding (BPE) subword tokens, `IdeamLinguistics`
 *      bridges traditional structural NLP and SLMs via **Grammar & Sememe Constrained Decoding**:
 *      * Pre-SLM: `IdeamMinds` selects the target Akashic Tome facts, `SememeID`s, and `LectID`.
 *      * During SLM Token Sampling: `SemanticLexiconCore` compiles the active `Grammar` and
 *        `Lect` vocabulary into a token logit mask so a local SLM cannot hallucinate lore or
 *        break character voice.
 *      * Post-SLM: Surface text emitted by the SLM is decomposed into `LexemePOD -> PhonemePOD`
 *        bitvectors and routed through `IdeamCommunications` for RPG skill checks and noisy-channel
 *        degradation.
 */

#include "../procedural_generation/procgen_common.h"
#include <array>
#include <bit>
#include <cstdint>

namespace ideam::core {

/**
 * 16-bit Jakobson-Chomsky-Halle articulatory feature bitmask for `PhonemePOD`.
 * Ordered from most noise-resilient (low bits: Syllabic/Sonorant/Voiced/Vowel Height)
 * to most noise-fragile under broadband masking (high bits: Place of Articulation).
 */
enum ArticulatoryFeatureBits : uint16_t {
    FEAT_NONE        = 0u,
    // --- Tier 1: Prosodic & Major Class Features (Highest Noise Resilience) ---
    FEAT_SYLLABIC    = 1u << 0,  // Vowel or syllabic consonant (+syllabic)
    FEAT_SONORANT    = 1u << 1,  // Vowels, glides, liquids, nasals (+sonorant)
    FEAT_VOICED      = 1u << 2,  // Vocal cord vibration (+voice: /b/, /d/, /g/, /v/, /z/)
    // --- Tier 2: Vowel Space Features (High-Medium Noise Resilience) ---
    FEAT_VOWEL_HIGH  = 1u << 3,  // High tongue body (/i/, /u/)
    FEAT_VOWEL_LOW   = 1u << 4,  // Low tongue body (/a/, /ae/)
    FEAT_VOWEL_BACK  = 1u << 5,  // Back tongue body (/u/, /o/, /a/)
    FEAT_VOWEL_ROUND = 1u << 6,  // Rounded lips (/u/, /o/)
    // --- Tier 3: Manner of Articulation Features (Medium Noise Resilience) ---
    FEAT_CONTINUANT  = 1u << 7,  // Fricatives/approximants (+cont: /f/, /s/) vs Stops (-cont: /p/, /t/)
    FEAT_NASAL       = 1u << 8,  // Nasal resonance (+nasal: /m/, /n/, /ng/)
    FEAT_STRIDENT    = 1u << 9,  // High-frequency turbulence (+strident: /s/, /z/, /sh/)
    FEAT_LATERAL     = 1u << 10, // Lateral airflow (+lateral: /l/)
    // --- Tier 4: Place of Articulation Features (Most Fragile in Noisy Environments) ---
    FEAT_LABIAL      = 1u << 11, // Bilabial / labiodental (/p/, /b/, /m/, /f/, /v/)
    FEAT_CORONAL     = 1u << 12, // Alveolar / dental / postalveolar (/t/, /d/, /s/, /n/)
    FEAT_ANTERIOR    = 1u << 13, // Front of alveolar ridge (+ant) vs postalveolar (-ant)
    FEAT_DORSAL      = 1u << 14, // Velar / palatal (/k/, /g/, /ng/)
    FEAT_GLOTTAL     = 1u << 15  // Laryngeal / glottal (/h/, /?/)
};

enum class MorphemeKind : uint8_t {
    FREE_ROOT          = 0,
    BOUND_PREFIX       = 1,
    BOUND_SUFFIX       = 2,
    INFLECTIONAL_AFFIX = 3,
    CLITIC             = 4
};

enum class LexicalCategory : uint8_t {
    UNDEFINED   = 0,
    NOUN        = 1,
    VERB        = 2,
    ADJECTIVE   = 3,
    ADVERB      = 4,
    ADPOSITION  = 5,
    PRONOUN     = 6,
    DETERMINER  = 7,
    CONJUNCTION = 8,
    PARTICLE    = 9
};

/**
 * PhonemePOD (16 Bytes, alignas(16))
 * Compact DOD representation of a single phoneme and its 16-bit articulatory feature matrix.
 */
struct alignas(16) PhonemePOD {
    uint32_t phoneme_id         = 0;
    uint32_t ipa_codepoint      = 0;   // Unicode IPA symbol (e.g. U+0070 'p', U+0283 'esh')
    uint16_t articulatory_mask  = 0;   // ArticulatoryFeatureBits
    uint16_t duration_ms        = 80;  // Typical phoneme duration for prosody timing
    uint8_t  sonority_rank      = 1;   // 1 (voiceless stop) .. 10 (low vowel) for syllable nuclei
    uint8_t  stress_level       = 0;   // 0 = unstressed, 1 = primary, 2 = secondary
    uint8_t  reserved[2]        = {0};

    [[nodiscard]] constexpr uint32_t feature_distance(uint16_t p_other_mask) const noexcept {
        return static_cast<uint32_t>(std::popcount(static_cast<uint16_t>(articulatory_mask ^ p_other_mask)));
    }
};
static_assert(sizeof(PhonemePOD) == 16, "PhonemePOD must be 16 bytes.");

/**
 * SemeDescriptorPOD (16 Bytes, alignas(16))
 * Atomic micro-component of meaning used in componential analysis.
 */
struct alignas(16) SemeDescriptorPOD {
    uint32_t seme_id            = 0;
    uint32_t name_hash          = 0;
    uint16_t domain_id          = 0;   // e.g., Spatial, Era, Genre, Emotion, PhysicalProperty
    uint16_t dimension_id       = 0;   // Axis within domain (e.g., PerspectiveAxis, MoralityAxis)
    uint16_t antonym_seme_id    = 0xFFFFu;
    uint8_t  is_nuclear_default = 1;
    uint8_t  reserved           = 0;
};
static_assert(sizeof(SemeDescriptorPOD) == 16, "SemeDescriptorPOD must be 16 bytes.");

/**
 * SememeDescriptorPOD (64 Bytes, alignas(64))
 * Bundle of Nuclear Semes (core invariant identity) and Peripheral Semes (contextual/connotative
 * shade) that defines a single unit of meaning across all Ideam plugins.
 */
struct alignas(64) SememeDescriptorPOD {
    std::array<uint64_t, 3> nuclear_seme_words{0ULL, 0ULL, 0ULL};    // 24B: 192 nuclear seme bits
    std::array<uint64_t, 3> peripheral_seme_words{0ULL, 0ULL, 0ULL}; // 24B: 192 peripheral seme bits
    uint32_t sememe_id          = 0;                                  // 4B
    uint32_t canonical_name_hash= 0;                                  // 4B
    float    emotive_valence    = 0.0f;                               // 4B: [-1.0 negative .. +1.0 positive]
    uint8_t  sememic_form       = 1;                                  // 1B: 1=Denotative, 2..5=Connotative
    uint8_t  taxeme_category    = 0;                                  // 1B
    uint16_t primary_domain_id  = 0;                                  // 2B -> 64B total
};
static_assert(sizeof(SememeDescriptorPOD) == 64, "SememeDescriptorPOD must be 64 bytes.");

/**
 * MorphemePOD (32 Bytes, alignas(16))
 * Minimal morphological unit linking form (phoneme slice) to meaning (seme delta mask).
 */
struct alignas(16) MorphemePOD {
    uint32_t morpheme_id          = 0;
    uint32_t form_string_hash     = 0;
    uint32_t phoneme_csr_offset   = 0;
    uint16_t phoneme_count        = 0;
    uint16_t primary_seme_id      = 0;
    uint64_t seme_modifier_mask   = 0ULL;
    MorphemeKind kind             = MorphemeKind::FREE_ROOT;
    uint8_t  reserved[7]          = {0};
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
    LexicalCategory category      = LexicalCategory::NOUN;
    uint8_t  register_formality   = 128; // 0 = slang/vulgar, 128 = neutral, 255 = high/archaic
    float    corpus_frequency     = 1.0f;
    uint32_t slm_bpe_token_id     = 0xFFFFFFFFu; // Optional mapping to local SLM tokenizer vocab
};
static_assert(sizeof(LexemePOD) == 32, "LexemePOD must be 32 bytes.");

} // namespace ideam::core
