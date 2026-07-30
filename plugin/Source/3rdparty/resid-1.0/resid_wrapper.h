/*
  ==============================================================================

    resid_wrapper.h
    Created: 2026
    Author:  Cline

  ==============================================================================
*/

#pragma once

/**
    reSID 1.0 namespace wrapper for backward compatibility with resid-0.16 API.

    This header provides a compatibility layer that wraps the reSID 1.0 library
    (which uses namespace reSID) and exposes it as namespace resid1 with
    backward-compatible type aliases and function signatures.

    Usage in PluginProcessor.h (when USE_RESID_1_0 is defined):
    @code
    #include "resid_wrapper.h"
    // Re-export types from resid1 namespace for use in plugin code
    using chip_model = resid1::chip_model;
    using sampling_method = resid1::sampling_method;
    using reg4 = resid1::reg4;
    using reg8 = resid1::reg8;
    using reg12 = resid1::reg12;
    using reg16 = resid1::reg16;
    using reg24 = resid1::reg24;
    using cycle_count = resid1::cycle_count;
    using SID = resid1::SID;
    // Enum values as constexpr for backward compatibility
    constexpr chip_model MOS6581 = resid1::MOS6581;
    constexpr chip_model MOS8580 = resid1::MOS8580;
    constexpr sampling_method SAMPLE_FAST = resid1::SAMPLE_FAST;
    constexpr sampling_method SAMPLE_INTERPOLATE = resid1::SAMPLE_INTERPOLATE;
    constexpr sampling_method SAMPLE_RESAMPLE = resid1::SAMPLE_RESAMPLE;
    constexpr sampling_method SAMPLE_RESAMPLE_FASTMEM = resid1::SAMPLE_RESAMPLE_FASTMEM;
    @endcode

    In the plugin code (PluginProcessor.cpp), use the same identifiers as resid-0.16:
    @code
    sid.set_chip_model(MOS6581);
    sid.set_sampling_parameters(1022730, SAMPLE_INTERPOLATE, sampleRate);
    @endcode

    API Differences between resid-0.16 and reSID 1.0:
    ================================================

    1. chip_model enum (reSID 1.0):
       - MOS6581  (same as resid-0.16)
       - MOS8580  (same as resid-0.16)

    2. sampling_method enum (reSID 1.0):
       - SAMPLE_FAST           (replaces SAMPLE_FAST from 0.16)
       - SAMPLE_INTERPOLATE    (replaces SAMPLE_INTERPOLATE from 0.16)
       - SAMPLE_RESAMPLE       (replaces SAMPLE_RESAMPLE from 0.16)
       - SAMPLE_RESAMPLE_FASTMEM (new name for SAMPLE_FAST_RESAMPLE from 0.16)

    3. SID class differences:
       - set_voice_mask()           - NEW in reSID 1.0
       - adjust_filter_bias()       - NEW in reSID 1.0
       - enable_raw_debug_output()  - NEW in reSID 1.0
       - input(short)               - CHANGED: int -> short in 0.16
       - output()                   - CHANGED: removed bits parameter in 0.16
       - debugoutput()              - NEW in reSID 1.0
       - State class has additional members in 1.0:
         * write_pipeline, write_address, voice_mask
         * shift_register_reset, shift_pipeline
         * pulse_output, floating_output_ttl, envelope_pipeline

    Note: The wrapper maintains the same interface as resid-0.16 for existing code.
    Any new features in reSID 1.0 that don't have equivalents in resid-0.16
    must be accessed directly via reSID:: namespace.
*/

// The include paths are relative to the resid-1.0 target's include directories,
// which add vice-git/vice/src/resid and the generated directory.
// Include siddefs.h first (defines types and namespace)
#include "siddefs.h"
// resid-config.h provides configuration macros (included by sid.h internally)
#include "sid.h"

namespace resid1
{
    // Re-export chip_model enum from reSID namespace
    using chip_model = reSID::chip_model;

    // Re-export sampling_method enum from reSID namespace
    using sampling_method = reSID::sampling_method;

    // Type aliases for SID registers
    using reg4 = reSID::reg4;
    using reg8 = reSID::reg8;
    using reg12 = reSID::reg12;
    using reg16 = reSID::reg16;
    using reg24 = reSID::reg24;
    using cycle_count = reSID::cycle_count;

    // Alias the reSID::SID class as resid1::SID
    using SID = reSID::SID;

    // Enum values for backward compatibility
    // chip_model values
    static constexpr chip_model MOS6581 = reSID::MOS6581;
    static constexpr chip_model MOS8580 = reSID::MOS8580;

    // sampling_method values
    static constexpr sampling_method SAMPLE_FAST = reSID::SAMPLE_FAST;
    static constexpr sampling_method SAMPLE_INTERPOLATE = reSID::SAMPLE_INTERPOLATE;
    static constexpr sampling_method SAMPLE_RESAMPLE = reSID::SAMPLE_RESAMPLE;
    static constexpr sampling_method SAMPLE_RESAMPLE_FASTMEM = reSID::SAMPLE_RESAMPLE_FASTMEM;

    // regToCutoff compatibility method
    // resid-0.16 had SID::regToCutoff(reg16 val) which returned cutoff frequency in Hz
    // reSID 1.0 now has SID::regToCutoff(reg16 val) which delegates to Filter::regToCutoff
    // This wrapper simply forwards the call to the SID class method
    inline int regToCutoff(SID& sid, reg16 val)
    {
        return sid.regToCutoff(val);
    }

    // resid_version_string is defined in siddefs.h
    // It's declared as extern "C" const char* resid_version_string in siddefs.h
    // and defined in version.cc as VERSION macro
    // Expose it through the resid1 namespace
    inline const char* getResidVersion()
    {
        return resid_version_string;
    }

} // namespace resid1
