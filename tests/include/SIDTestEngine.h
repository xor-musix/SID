/*
    ==============================================================================

    SIDTestEngine.h
    Created: 2026
    Author:  Test Harness

    ==============================================================================
*/

#pragma once

#include "../../plugin/Source/PluginProcessor.h"
#include <JuceHeader.h>
#include <fstream>

/**
    Test wrapper for SID engine that allows:
    - Headless rendering of audio
    - Parameter control
    - Audio output capture
    - Multiple engine comparison
*/
class SIDTestEngine : public SIDAudioProcessor
{
public:
    SIDTestEngine()
        : SIDAudioProcessor()
    {
        // Initialize with default parameters
        prepareToPlay(48000.0, 512);
    }

    ~SIDTestEngine() override = default;

    // Set a parameter value by ID
    void setParameter(const juce::String& paramId, float value)
    {
        if (auto* param = findParameter(paramId))
        {
            param->setValue(value);
        }
    }

    // Find a parameter by ID
    juce::AudioProcessorParameter* findParameter(const juce::String& paramId)
    {
        for (auto* param : getParameters())
        {
            if (param->getName(128) == paramId)
                return param;
        }
        return nullptr;
    }

    // Get parameter value
    float getParameter(const juce::String& paramId)
    {
        if (auto* param = findParameter(paramId))
        {
            return param->getValue();
        }
        return 0.0f;
    }

    // Render a single block of audio
    void renderBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midi)
    {
        // Clear buffer first
        buffer.clear();

        // Process the block
        processBlock(buffer, midi);

        // Get output from fifo
        if (fifo.getFreeSpace() >= buffer.getNumSamples())
        {
            fifo.readMono(buffer.getWritePointer(0), buffer.getNumSamples());
        }
    }

    // Render multiple blocks to an output buffer
    void renderToBuffer(juce::AudioSampleBuffer& outputBuffer,
                        const juce::MidiBuffer& midi,
                        int startSample = 0,
                        int numSamples = -1)
    {
        if (numSamples < 0)
            numSamples = outputBuffer.getNumSamples();

        int samplesToRender = numSamples;
        int samplePos = startSample;

        while (samplesToRender > 0)
        {
            int blockSize = std::min(samplesToRender, 512);
            juce::AudioSampleBuffer buffer(1, blockSize);
            buffer.clear();

            // Clear any pending MIDI
            juce::MidiBuffer midiBlock;

            // Extract MIDI for this block
            for (const auto metadata : midi)
            {
                auto msg = metadata.getMessage();
                int pos = metadata.samplePosition;

                // Only include MIDI that falls in this block
                if (pos >= samplePos && pos < samplePos + blockSize)
                {
                    // Adjust position relative to block start
                    midiBlock.addEvent(msg, pos - samplePos);
                }
            }

            // Process this block
            processBlock(buffer, midiBlock);

            // Copy to output
            outputBuffer.copyFrom(0, samplePos, buffer, 0, 0, blockSize);

            samplePos += blockSize;
            samplesToRender -= blockSize;
        }
    }

    // Get the current SID engine name
    juce::String getEngineName() const
    {
        return getSidEmulatorName();
    }

    // Get the reSID version
    juce::String getVersion() const
    {
        return getResidVersion();
    }

    // Reset all voices
    void resetVoices()
    {
        reset();
    }

    // Play a note on a specific channel
    void playNote(int noteNumber, int velocity = 127, int channel = 0)
    {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(channel + 1, noteNumber, juce::uint8(velocity)), 0);
        processBlock(audioBuffer, midi);
    }

    // Stop a note
    void stopNote(int noteNumber, int channel = 0)
    {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOff(channel + 1, noteNumber), 0);
        processBlock(audioBuffer, midi);
    }

    // Generate a test tone and capture output
    void generateTestTone(float frequency, float durationSeconds,
                         juce::AudioSampleBuffer& output,
                         int sampleRate = 48000)
    {
        int numSamples = static_cast<int>(durationSeconds * sampleRate);
        output.setSize(1, numSamples, false, true, true);

        // Set up oscillator parameters
        setParameter(paramWave1, 2.0f);  // Saw wave
        setParameter(paramTune1, 0.0f);
        setParameter(paramA1, 1.0f);    // Fast attack
        setParameter(paramD1, 4.0f);    // Medium decay
        setParameter(paramS1, 8.0f);    // Medium sustain
        setParameter(paramR1, 4.0f);    // Medium release
        setParameter(paramVol, 12.0f);  // Good volume

        // Calculate period from frequency
        // freq = (clock / 14) * (period / 2^24)
        // period = freq * 2^24 * 14 / clock
        double clockFreq = 1022730.0;  // NTSC clock
        double period = frequency * std::pow(2.0, 24) * 14.0 / clockFreq;

        // Set oscillator frequency
        int periodInt = static_cast<int>(period);
        setRegister(0x00, periodInt & 0xFF);
        setRegister(0x01, (periodInt >> 8) & 0xFF);

        // Set waveform on
        setRegister(0x04, 0x10);  // Saw wave + enable

        // Render audio
        int samplesDone = 0;
        int samplesLeft = numSamples;

        while (samplesLeft > 0)
        {
            int blockSize = std::min(samplesLeft, 512);
            juce::AudioSampleBuffer buffer(1, blockSize);
            buffer.clear();

            juce::MidiBuffer emptyMidi;
            processBlock(buffer, emptyMidi);

            output.copyFrom(0, samplesDone, buffer, 0, 0, blockSize);

            samplesDone += blockSize;
            samplesLeft -= blockSize;
        }

        // Turn off oscillator
        setRegister(0x04, 0x00);
    }

    // Write a SID register directly
    void setRegister(uint8_t reg, uint8_t value)
    {
        if (getNumVoices() > 0)
        {
            getVoice(0)->testWriteReg(reg, value);
        }
    }

    // Get a SID register value from cache
    uint8_t getRegister(uint8_t reg)
    {
        if (getNumVoices() > 0)
        {
            return getVoice(0)->getRegCache().at(reg);
        }
        return 0;
    }

    // Get number of active voices
    int getActiveVoices() const
    {
        int count = 0;
        for (int i = 0; i < getNumVoices(); ++i)
        {
            if (getVoice(i)->getNote() >= 0)
                count++;
        }
        return count;
    }

    // Get note on a specific voice
    int getVoiceNote(int voiceIndex) const
    {
        if (voiceIndex >= 0 && voiceIndex < getNumVoices())
            return getVoice(voiceIndex)->getNote();
        return -1;
    }

    // Get register cache for all voices
    std::map<uint8_t, uint8_t> getVoice0RegisterCache() const
    {
        if (getNumVoices() > 0)
            return getVoice(0)->getRegCache();
        return {};
    }

    // Write a simple WAV file using standard C++ (no JUCE dependencies)
    void exportAudio(const juce::AudioSampleBuffer& buffer, const juce::String& filename)
    {
        // Create WAV file using minimal implementation
        // WAV format: 44-byte header + PCM data
        
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();
        int sampleRate = 48000;  // Default sample rate
        
        // Create output directory if it doesn't exist
        juce::File fileObj(filename);
        juce::File dir = fileObj.getParentDirectory();
        if (!dir.exists())
            dir.createDirectory();
        
        // Write WAV header
        std::ofstream file(filename.toUTF8(), std::ios::binary);
        if (!file)
        {
            std::cerr << "Failed to open file: " << filename.toStdString() << std::endl;
            return;
        }
        
        // RIFF header
        file.write("RIFF", 4);
        int32_t fileSize = 36 + numSamples * numChannels * 2;  // 16-bit PCM
        file.write(reinterpret_cast<const char*>(&fileSize), 4);
        file.write("WAVE", 4);
        
        // fmt chunk
        file.write("fmt ", 4);
        int32_t fmtSize = 16;  // PCM format
        file.write(reinterpret_cast<const char*>(&fmtSize), 4);
        int16_t audioFormat = 1;  // PCM
        file.write(reinterpret_cast<const char*>(&audioFormat), 2);
        int16_t channels = static_cast<int16_t>(numChannels);
        file.write(reinterpret_cast<const char*>(&channels), 2);
        int32_t sampleRate32 = sampleRate;
        file.write(reinterpret_cast<const char*>(&sampleRate32), 4);
        int32_t byteRate = sampleRate * numChannels * 2;  // 16-bit
        file.write(reinterpret_cast<const char*>(&byteRate), 4);
        int16_t blockAlign = numChannels * 2;
        file.write(reinterpret_cast<const char*>(&blockAlign), 2);
        int16_t bitsPerSample = 16;
        file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
        
        // data chunk
        file.write("data", 4);
        int32_t dataSize = numSamples * numChannels * 2;
        file.write(reinterpret_cast<const char*>(&dataSize), 4);
        
        // Write PCM data (convert float to 16-bit)
        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float sample = buffer.getSample(ch, i);
                // Clamp to [-1, 1]
                sample = juce::jlimit(-1.0f, 1.0f, sample);
                // Convert to 16-bit
                int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
                file.write(reinterpret_cast<const char*>(&pcm), 2);
            }
        }
        
        file.close();
    }

    // Compare two audio buffers and return similarity metrics
    struct AudioComparison
    {
        double correlation = 0.0;    // Pearson correlation (-1 to 1)
        double snr = 0.0;           // Signal-to-noise ratio in dB
        double distortion = 0.0;    // Total harmonic distortion %
        double maxDiff = 0.0;       // Maximum sample difference
        double meanDiff = 0.0;      // Mean absolute difference
    };

    AudioComparison compareBuffers(const juce::AudioSampleBuffer& ref,
                                   const juce::AudioSampleBuffer& test)
    {
        AudioComparison result;

        if (ref.getNumSamples() != test.getNumSamples() ||
            ref.getNumChannels() != test.getNumChannels())
        {
            return result;
        }

        int numSamples = ref.getNumSamples();
        int numChannels = ref.getNumChannels();

        double sumRef = 0.0, sumTest = 0.0;
        double sumRefSq = 0.0, sumTestSq = 0.0;
        double sumRefTest = 0.0;
        double sumDiffSq = 0.0;
        double maxDiff = 0.0;
        double sumAbsDiff = 0.0;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* refData = ref.getReadPointer(ch);
            const float* testData = test.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                double refVal = refData[i];
                double testVal = testData[i];

                sumRef += refVal;
                sumTest += testVal;
                sumRefSq += refVal * refVal;
                sumTestSq += testVal * testVal;
                sumRefTest += refVal * testVal;

                double diff = refVal - testVal;
                sumDiffSq += diff * diff;
                sumAbsDiff += std::abs(diff);
                maxDiff = std::max(maxDiff, std::abs(diff));
            }
        }

        // Pearson correlation
        double n = numSamples * numChannels;
        double meanRef = sumRef / n;
        double meanTest = sumTest / n;

        double cov = (sumRefTest / n) - (meanRef * meanTest);
        double stdRef = std::sqrt((sumRefSq / n) - (meanRef * meanRef));
        double stdTest = std::sqrt((sumTestSq / n) - (meanTest * meanTest));

        if (stdRef > 0.0 && stdTest > 0.0)
        {
            result.correlation = cov / (stdRef * stdTest);
        }

        // SNR (assuming reference is signal, difference is noise)
        double signalPower = (sumRefSq / n);
        double noisePower = (sumDiffSq / n);

        if (noisePower > 0.0)
        {
            result.snr = 10.0 * std::log10(signalPower / noisePower);
        }

        // Mean absolute difference
        result.meanDiff = sumAbsDiff / n;
        result.maxDiff = maxDiff;

        return result;
    }

    // Save comparison results to a JSON file
    void saveComparisonResults(const AudioComparison& comp,
                               const juce::String& engineA,
                               const juce::String& engineB,
                               const juce::String& filename)
    {
        juce::File file(filename);

        std::unique_ptr<juce::OutputStream> stream(file.createOutputStream());
        if (stream != nullptr)
        {
            juce::String json = R"({
    "comparison": {
        "engineA": ")" + engineA + R"(",
        "engineB": ")" + engineB + R"(",
        "metrics": {
            "correlation": )" + juce::String(comp.correlation, 6) + R"(,
            "snr_db": )" + juce::String(comp.snr, 2) + R"(,
            "max_diff": )" + juce::String(comp.maxDiff, 8) + R"(,
            "mean_diff": )" + juce::String(comp.meanDiff, 8) + R"(
        }
    }
})";

            stream->write(json.toRawUTF8(), json.getNumBytesAsUTF8());
            stream->flush();
            stream.reset();
        }
    }

    // Generate a simple MIDI sequence for testing
    juce::MidiBuffer generateMidiSequence(const juce::Array<int>& notes,
                                          float noteDuration,
                                          int sampleRate)
    {
        juce::MidiBuffer midi;
        int samplesPerNote = static_cast<int>(noteDuration * sampleRate);

        for (int i = 0; i < notes.size(); ++i)
        {
            int note = notes[i];
            int startSample = i * samplesPerNote;

            midi.addEvent(juce::MidiMessage::noteOn(1, note, juce::uint8(100)), startSample);
            midi.addEvent(juce::MidiMessage::noteOff(1, note), startSample + samplesPerNote / 2);
        }

        return midi;
    }

    // Generate a test patch for a specific engine
    void setTestPatch(const juce::String& patchName)
    {
        // Default patch settings
        setParameter(paramWave1, 2.0f);   // Saw
        setParameter(paramWave2, 1.0f);   // Triangle
        setParameter(paramWave3, 0.0f);   // Off
        setParameter(paramA1, 4.0f);
        setParameter(paramD1, 6.0f);
        setParameter(paramS1, 10.0f);
        setParameter(paramR1, 6.0f);
        setParameter(paramA2, 4.0f);
        setParameter(paramD2, 6.0f);
        setParameter(paramS2, 10.0f);
        setParameter(paramR2, 6.0f);
        setParameter(paramVol, 12.0f);
        setParameter(paramCutoff, 1024.0f);
        setParameter(paramReso, 8.0f);
        setParameter(paramLP, 1.0f);
        setParameter(paramBP, 0.0f);
        setParameter(paramHP, 0.0f);
        setParameter(paramVoices, 1.0f);

        if (patchName == "pulse")
        {
            setParameter(paramWave1, 3.0f);   // Square
            setParameter(paramPulseWidth1, 2048.0f);
        }
        else if (patchName == "bass")
        {
            setParameter(paramWave1, 1.0f);   // Triangle
            setParameter(paramA1, 1.0f);      // Fast attack
            setParameter(paramD1, 10.0f);
            setParameter(paramS1, 5.0f);
            setParameter(paramR1, 2.0f);
        }
        else if (patchName == "pad")
        {
            setParameter(paramWave1, 1.0f);   // Triangle
            setParameter(paramWave2, 2.0f);   // Saw
            setParameter(paramA1, 10.0f);     // Slow attack
            setParameter(paramD1, 8.0f);
            setParameter(paramS1, 12.0f);
            setParameter(paramR1, 10.0f);
            setParameter(paramSync1, 1.0f);
        }
    }

private:
    juce::AudioSampleBuffer audioBuffer{1, 512};
};