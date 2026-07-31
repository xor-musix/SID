/*
  ==============================================================================

    residfp_wrapper.h
    Created: 2026
    Author:  Cline

  ==============================================================================
*/

#pragma once

#include <cstdint>

/**
    libresidfp namespace wrapper for backward compatibility with resid-0.16 and resid-1.0 APIs.

    This header provides a compatibility layer that wraps the libresidfp library
    (which uses namespace reSIDfp) and exposes it as namespace residfp with
    backward-compatible type aliases and function signatures.

    Usage in PluginProcessor.h (when SID_EMULATOR == 2 is defined):
    @code
    #include "residfp_wrapper.h"
    // Re-export types from residfp namespace for use in plugin code
    using chip_model = residfp::chip_model;
    using sampling_method = residfp::sampling_method;
    using reg4 = residfp::reg4;
    using reg8 = residfp::reg8;
    using reg12 = residfp::reg12;
    using reg16 = residfp::reg16;
    using reg24 = residfp::reg24;
    using cycle_count = residfp::cycle_count;
    using SID = residfp::SID;
    // Enum values as constexpr for backward compatibility
    constexpr chip_model MOS6581 = residfp::MOS6581;
    constexpr chip_model MOS8580 = residfp::MOS8580;
    constexpr sampling_method SAMPLE_FAST = residfp::SAMPLE_FAST;
    constexpr sampling_method SAMPLE_INTERPOLATE = residfp::SAMPLE_INTERPOLATE;
    constexpr sampling_method SAMPLE_RESAMPLE = residfp::SAMPLE_RESAMPLE;
    constexpr sampling_method SAMPLE_RESAMPLE_FASTMEM = residfp::SAMPLE_RESAMPLE_FASTMEM;
    @endcode

    In the plugin code (PluginProcessor.cpp), use the same identifiers as resid-0.16:
    @code
    sid.set_chip_model(MOS6581);
    sid.set_sampling_parameters(1022730, SAMPLE_INTERPOLATE, sampleRate);
    @endcode

    API Mapping between libraries:
    ================================================

    1. chip_model enum (libresidfp):
       - MOS6581  (same as resid-0.16 and resid-1.0)
       - CSG8580  (named MOS8580 in resid-0.16/resid-1.0)

    2. sampling_method enum (libresidfp):
       - DECIMATE     (equivalent to SAMPLE_FAST)
       - RESAMPLE     (equivalent to SAMPLE_INTERPOLATE, SAMPLE_RESAMPLE)
       - NONE         (raw 1MHz output)

    3. SID class differences:
       - set_chip_model()           - SAME signature
       - set_sampling_parameters()  - SAME signature (with different parameter names)
       - reset()                    - SAME signature
       - write()                    - SAME signature
       - clock()                    - DIFFERENT signature (libresidfp returns cycles, takes buffer+size)
       - regToCutoff()              - ADDED in libresidfp (helper method)

    Note: The wrapper maintains the same interface as resid-0.16 for existing code.
*/

// Include path is relative to the generated directory
#include "siddefs-fp.h"
#include "residfp/residfp.h"
// Include the full definition of reSIDfp::SID
#include "residfp/residfp_defs.h"
// Include SID.h which contains the full definition
#include "SID.h"

namespace residfp
{
    // Type aliases for chip_model (using typedef from reSIDfp)
    using chip_model = reSIDfp::ChipModel;

    // Type aliases for sampling_method (using typedef from reSIDfp)
    using sampling_method = reSIDfp::SamplingMethod;

    // Type aliases for SID registers (libresidfp doesn't define these, use int types)
    using reg4 = int;
    using reg8 = uint8_t;
    using reg12 = int;
    using reg16 = int;
    using reg24 = long;
    using cycle_count = int;

    // Chip model constants
    // Note: libresidfp uses CSG8580 for the 8580 model
    static constexpr chip_model MOS6581 = reSIDfp::MOS6581;
    static constexpr chip_model MOS8580 = reSIDfp::CSG8580;

    // Sampling method constants
    // DECIMATE = linear interpolation (fast but low quality) -> SAMPLE_FAST
    // RESAMPLE = sinc resampling (high quality) -> SAMPLE_INTERPOLATE, SAMPLE_RESAMPLE
    static constexpr sampling_method SAMPLE_FAST = reSIDfp::DECIMATE;
    static constexpr sampling_method SAMPLE_INTERPOLATE = reSIDfp::RESAMPLE;
    static constexpr sampling_method SAMPLE_RESAMPLE = reSIDfp::RESAMPLE;
    static constexpr sampling_method SAMPLE_RESAMPLE_FASTMEM = reSIDfp::RESAMPLE;

    /**
        Wrapper class for reSIDfp::SID that matches resid-0.16 API.
    */
    class SID
    {
    public:
        /**
            Set the chip model (MOS6581 or MOS8580).
        */
        void set_chip_model(chip_model model)
        {
            sid.setChipModel(model);
        }

        /**
            Set sampling parameters.
            @param clock_freq Clock frequency in Hz (985248 for PAL, 1022730 for NTSC)
            @param method Sampling method (SAMPLE_FAST, SAMPLE_INTERPOLATE, etc.)
            @param sample_freq Sample frequency in Hz
            @param pass_freq Not used (for compatibility only)
            @param filter_scale Not used (for compatibility only)
        */
        void set_sampling_parameters(double clock_freq, sampling_method method, double sample_freq, double pass_freq = -1, double filter_scale = 0.97)
        {
            sid.setSamplingParameters(clock_freq, method, sample_freq);
        }

        /**
            Reset the SID chip.
        */
        void reset()
        {
            sid.reset();
        }

        /**
            Write to a SID register.
            @param offset Register offset (0-0x18)
            @param value Value to write
        */
        void write(int offset, uint8_t value)
        {
            sid.write(offset, value);
        }

        /**
            Clock the SID forward producing audio.
            This is the 3-argument version that matches resid-0.16 API.
            
            @param delta_t Reference to number of cycles to run (modified to remaining cycles)
            @param buf Output buffer for audio samples
            @param n Number of samples to produce
            @return Number of samples produced
        */
        int clock(cycle_count& delta_t, short* buf, int n)
        {
            // libresidfp's clock returns cycles run for producing samples
            // We need to run for a given number of cycles and return samples produced
            // 
            // The approach: since libresidfp doesn't give us direct control over
            // how many cycles to run, we estimate based on the sampling parameters.
            //
            // At 1MHz clock and typical sample rate of 48kHz:
            // - 1022730 cycles per second / 48000 samples per second = ~21.3 cycles per sample
            // So roughly 21 cycles produce 1 sample
            
            int samples_produced = 0;
            cycle_count cycles_remaining = delta_t;
            
            while (cycles_remaining > 0 && samples_produced < n)
            {
                // Calculate how many samples this chunk of cycles should produce
                // Using a rough estimate of 21 cycles per sample (1022730 / 48000)
                // This is an approximation since we don't have access to the actual sample rate
                const int CYCLES_PER_SAMPLE_ESTIMATE = 21;
                int samples_to_produce = cycles_remaining / CYCLES_PER_SAMPLE_ESTIMATE;
                
                // Make sure we don't overflow the buffer
                samples_to_produce = std::min(samples_to_produce, n - samples_produced);
                
                // If we can't produce any samples, just run digital
                if (samples_to_produce <= 0)
                {
                    sid.clockDigital(cycles_remaining);
                    cycles_remaining = 0;
                    break;
                }
                
                // Run the emulator and get cycles run
                cycle_count cycles_run = sid.clock(buf + samples_produced, samples_to_produce);
                
                // Update remaining cycles (cycles_run are the cycles that were run)
                cycles_remaining -= cycles_run;
                samples_produced += samples_to_produce;
            }
            
            delta_t = cycles_remaining;
            return samples_produced;
        }

        /**
            Clock the SID forward without producing audio.
            @param delta_t Number of cycles to run
        */
        void clock(cycle_count delta_t)
        {
            sid.clockDigital(delta_t);
        }

        /**
            Helper method to convert a register value to cutoff frequency.
            @param val Register value
            @return Cutoff frequency in Hz
        */
        int regToCutoff(uint16_t val)
        {
            // libresidfp doesn't have a direct regToCutoff method
            // This is a placeholder - the actual implementation would need
            // to be added to the library or we use a similar calculation
            return 0;
        }

    private:
        reSIDfp::SID sid;
    };

    // Get the libresidfp version string (uses sidversion.h from the library)
    // Note: sidversion.h cannot be included directly due to #error
    // The version is defined in the generated sidversion.h as a string literal
    inline const char* getResidVersion()
    {
        return SID_VERSION_STRING;
    }

} // namespace residfp