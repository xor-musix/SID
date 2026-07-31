/*
  ==============================================================================

    residfp_wrapper.h
    Created: 2026
    Author:  Cline

  ==============================================================================
*/

#pragma once

#include <cstdint>
#include <cassert>

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

// siddefs-fp.h must be included first to define types and version string
// This file is generated from siddefs-fp.h.in by CMake
#include "siddefs-fp.h"

// Include SID.h first (includes all the reSIDfp headers properly)
// SID.h includes residfp/residfp_defs.h and siddefs-fp.h internally
#include "SID.h"

// residfp.h defines the reSIDfp namespace and class declarations
// It also includes sidversion.h which requires RESIDFP_H to be defined
// Since SID.h already includes everything we need, we don't need to include residfp.h again

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
        Cutoff frequency lookup tables for MOS6581 and MOS8580.
        These tables map FC register values (0-2047) to cutoff frequencies in Hz.
        The tables are taken from resid-0.16 filter.cc.
    */
    constexpr int f0_points_6581[][2] =
    {
        {    0,   220 },   // 0x00      - repeated end point
        {    0,   220 },   // 0x00
        {  128,   230 },   // 0x10
        {  256,   250 },   // 0x20
        {  384,   300 },   // 0x30
        {  512,   420 },   // 0x40
        {  640,   780 },   // 0x50
        {  768,  1600 },   // 0x60
        {  832,  2300 },   // 0x68
        {  896,  3200 },   // 0x70
        {  960,  4300 },   // 0x78
        {  992,  5000 },   // 0x7c
        { 1008,  5400 },   // 0x7e
        { 1016,  5700 },   // 0x7f
        { 1023,  6000 },   // 0x7f 0x07
        { 1023,  6000 },   // 0x7f 0x07 - discontinuity
        { 1024,  4600 },   // 0x80      -
        { 1024,  4600 },   // 0x80
        { 1032,  4800 },   // 0x81
        { 1056,  5300 },   // 0x84
        { 1088,  6000 },   // 0x88
        { 1120,  6600 },   // 0x8c
        { 1152,  7200 },   // 0x90
        { 1280,  9500 },   // 0xa0
        { 1408, 12000 },   // 0xb0
        { 1536, 14500 },   // 0xc0
        { 1664, 16000 },   // 0xd0
        { 1792, 17100 },   // 0xe0
        { 1920, 17700 },   // 0xf0
        { 2047, 18000 },   // 0xff 0x07
        { 2047, 18000 }    // 0xff 0x07 - repeated end point
    };

    constexpr int f0_points_8580[][2] =
    {
        {    0,     0 },   // 0x00      - repeated end point
        {    0,     0 },   // 0x00
        {  128,   800 },   // 0x10
        {  256,  1600 },   // 0x20
        {  384,  2500 },   // 0x30
        {  512,  3300 },   // 0x40
        {  640,  4100 },   // 0x50
        {  768,  4800 },   // 0x60
        {  896,  5600 },   // 0x70
        { 1024,  6500 },   // 0x80
        { 1152,  7500 },   // 0x90
        { 1280,  8400 },   // 0xa0
        { 1408,  9200 },   // 0xb0
        { 1536,  9800 },   // 0xc0
        { 1664, 10500 },   // 0xd0
        { 1792, 11000 },   // 0xe0
        { 1920, 11700 },   // 0xf0
        { 2047, 12500 }    // 0xff 0x07 - repeated end point
    };

    /**
        Linear interpolation helper to find cutoff frequency for a given FC value.
        @param points Array of {FC, frequency} points
        @param num_points Number of points in the array
        @param fc_value FC register value (0-2047)
        @return Cutoff frequency in Hz
    */
    inline int interpolate_fc(const int points[][2], int num_points, int fc_value)
    {
        if (fc_value <= points[0][0])
            return points[0][1];
        if (fc_value >= points[num_points-1][0])
            return points[num_points-1][1];

        for (int i = 0; i < num_points - 1; i++)
        {
            if (fc_value >= points[i][0] && fc_value <= points[i+1][0])
            {
                int x0 = points[i][0];
                int y0 = points[i][1];
                int x1 = points[i+1][0];
                int y1 = points[i+1][1];
                // Linear interpolation
                return y0 + (y1 - y0) * (fc_value - x0) / (x1 - x0);
            }
        }
        return points[num_points-1][1];
    }

    /**
        Wrapper class for reSIDfp::SID that matches resid-0.16 API.

        The key difference is the clock() method:
        - resid-0.16: clock(cycle_count& delta_t, short* buf, int n)
          Takes a reference to cycles, returns samples produced, modifies delta_t to remaining cycles
        - libresidfp: clock(int16_t* buf, int bufSize)
          Returns cycles run, produces up to bufSize samples

        The wrapper implements the resid-0.16 signature by:
        1. Calling the libresidfp clock(buf, n) which produces up to n samples
        2. Estimating how many cycles were consumed based on the ratio of samples produced
    */
    class SID
    {
    public:
        SID() : chip_model_(MOS6581) {}

        /**
            Set the chip model (MOS6581 or MOS8580).
        */
        void set_chip_model(chip_model model)
        {
            chip_model_ = model;
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
            @param n Number of samples to produce (max)
            @return Number of samples produced
        */
        int clock(cycle_count& delta_t, short* buf, int n)
        {
            // The libresidfp clock(int16_t* buf, int bufSize) produces up to bufSize samples
            // and returns the number of cycles that were run.
            // 
            // The resid-0.16 clock(cycle_count& delta_t, short* buf, int n) runs for delta_t cycles
            // and returns the number of samples produced, modifying delta_t to remaining cycles.
            //
            // To implement resid-0.16 behavior with libresidfp:
            // 1. First, run digital clocking for delta_t cycles
            // 2. Then run audio-producing clock for remaining cycles
            
            // First, run the digital part (no audio output)
            sid.clockDigital(delta_t);
            
            // Now we need to produce n samples based on how many cycles that represents
            // At 1MHz clock and typical sample rate of 48kHz:
            // - 1022730 cycles per second / 48000 samples per second = ~21.3 cycles per sample
            // We use a rough estimate of 21 cycles per sample
            
            const double CYCLES_PER_SAMPLE = 1022730.0 / 48000.0;  // ~21.3
            
            // Estimate how many samples delta_t cycles would produce
            int estimated_samples = static_cast<int>(delta_t / CYCLES_PER_SAMPLE);
            
            // Make sure we don't overflow the buffer
            int samples_to_produce = std::min(estimated_samples, n);
            
            if (samples_to_produce <= 0)
            {
                // If no samples can be produced, return 0 and consume all cycles
                delta_t = 0;
                return 0;
            }
            
            // Run audio-producing clock for samples_to_produce samples
            // This returns the number of cycles that were run
            int cycles_run = sid.clock(buf, samples_to_produce);
            
            // Update remaining cycles
            delta_t = cycles_run;
            
            return samples_to_produce;
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
            @param val Register value (FC register, 0-2047)
            @return Cutoff frequency in Hz
        */
        int regToCutoff(uint16_t val)
        {
            // Linear interpolation between tabulated points
            if (chip_model_ == MOS6581)
            {
                return interpolate_fc(f0_points_6581, 
                    sizeof(f0_points_6581)/sizeof(*f0_points_6581), val);
            }
            else
            {
                return interpolate_fc(f0_points_8580,
                    sizeof(f0_points_8580)/sizeof(*f0_points_8580), val);
            }
        }

    private:
        reSIDfp::SID sid;
        chip_model chip_model_;
    };

    // Get the libresidfp version string
    // The version string is defined in siddefs-fp.h
    inline const char* getResidVersion()
    {
        return residfp_version_string;
    }

} // namespace residfp