/*
    ==============================================================================

    tests_main.cpp
    Created: 2026
    Author:  Test Harness

    ==============================================================================
*/

#include <JuceHeader.h>
#include "include/SIDTestEngine.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// Test configuration
struct TestConfig
{
    juce::String engineName;
    juce::String outputDir;
    float duration = 2.0f;
    int sampleRate = 48000;
    juce::Array<int> testNotes = {60, 64, 67, 72};  // C major chord
};

// Forward declarations
void runOscillatorTest(const TestConfig& config);
void runFilterTest(const TestConfig& config);
void runEnvelopeTest(const TestConfig& config);
void runPolyphonyTest(const TestConfig& config);
void compareEngines(const juce::String& dirA, const juce::String& dirB);
void printUsage();

//==============================================================================
void printUsage()
{
    std::cout << R"(
SID Test Harness - Audio Output Comparison Tool

Usage:
    SID_tests [options] test <type>       Run tests
    SID_tests compare <dirA> <dirB>  Compare test results

Options:
    --output-dir <dir>  Set output directory for test results (default: test_output)

Test Types:
    all         Run all tests
    oscillators Test oscillator waveforms and frequencies
    filters     Test filter responses
    envelopes   Test ADSR envelopes
    polyphony   Test multi-voice behavior

Comparison:
    Compares audio files in two directories and outputs metrics.

Examples:
    SID_tests test all
    SID_tests test oscillators
    SID_tests compare results/resid016 results/resid10
)" << std::endl;
}

//==============================================================================
int main(int argc, char* argv[])
{
    juce::String commandLine;
    juce::String outputDir = "test_output";

    // Parse command line arguments for --output-dir option
    juce::StringArray args;
    args.addTokens(commandLine, false);

    // Build args from command line, extracting --output-dir
    juce::StringArray processedArgs;
    for (int i = 1; i < argc; ++i)
    {
        juce::String arg = argv[i];
        if (arg == "--output-dir" && i + 1 < argc)
        {
            outputDir = argv[i + 1];
            ++i;  // Skip the next argument
        }
        else
        {
            processedArgs.add(arg);
        }
    }

    // Rebuild command line without --output-dir arguments
    commandLine = "";
    for (int i = 0; i < processedArgs.size(); ++i)
    {
        if (i > 0)
            commandLine += " ";
        commandLine += processedArgs[i];
    }

    args.addTokens(commandLine, false);

    if (args.size() < 2)
    {
        printUsage();
        return 0;
    }

    juce::String command = args[0].toLowerCase();

    if (command == "help" || command == "--help" || command == "-h")
    {
        printUsage();
        return 0;
    }
    else if (command == "test")
    {
        if (args.size() < 2)
        {
            std::cerr << "Error: Test name required" << std::endl;
            printUsage();
            return 1;
        }

        juce::String testType = args[1];
        TestConfig config;
        config.engineName = "Test";
        config.outputDir = outputDir;

        if (testType == "all")
        {
            runOscillatorTest(config);
            runFilterTest(config);
            runEnvelopeTest(config);
            runPolyphonyTest(config);
        }
        else if (testType == "oscillators")
        {
            runOscillatorTest(config);
        }
        else if (testType == "filters")
        {
            runFilterTest(config);
        }
        else if (testType == "envelopes")
        {
            runEnvelopeTest(config);
        }
        else if (testType == "polyphony")
        {
            runPolyphonyTest(config);
        }
        else
        {
            std::cerr << "Unknown test type: " << testType << std::endl;
            printUsage();
            return 1;
        }
    }
    else if (command == "compare")
    {
        if (args.size() < 3)
        {
            std::cerr << "Error: Two directories required for comparison" << std::endl;
            printUsage();
            return 1;
        }

        juce::String dirA = args[1];
        juce::String dirB = args[2];
        compareEngines(dirA, dirB);
    }
    else
    {
        std::cerr << "Unknown command: " << command << std::endl;
        printUsage();
        return 1;
    }

    return 0;
}

//==============================================================================
// Test implementations

void runOscillatorTest(const TestConfig& config)
{
    std::cout << "\n=== Oscillator Test ===" << std::endl;

    // Create test engine
    auto engine = std::make_unique<SIDTestEngine>();

    std::cout << "Engine: " << engine->getEngineName().toStdString() << std::endl;
    std::cout << "Version: " << engine->getVersion().toStdString() << std::endl;

    // Test each waveform
    const char* waveforms[] = {"Triangle", "Saw", "Square", "Noise"};
    const float waveValues[] = {1.0f, 2.0f, 3.0f, 4.0f};

    for (int w = 0; w < 4; ++w)
    {
        std::cout << "\nTesting " << waveforms[w] << "..." << std::endl;

        // Set waveform
        engine->setParameter(SIDAudioProcessor::paramWave1, waveValues[w]);
        engine->setParameter(SIDAudioProcessor::paramA1, 1.0f);
        engine->setParameter(SIDAudioProcessor::paramD1, 4.0f);
        engine->setParameter(SIDAudioProcessor::paramS1, 8.0f);
        engine->setParameter(SIDAudioProcessor::paramR1, 4.0f);
        engine->setParameter(SIDAudioProcessor::paramVol, 12.0f);

        // Generate 1-second test tone
        juce::AudioSampleBuffer buffer(1, config.sampleRate * config.duration);
        buffer.clear();

        engine->generateTestTone(440.0f, config.duration, buffer, config.sampleRate);

        // Export audio
        juce::String filename = config.outputDir + "/osc_" + juce::String(waveforms[w]) + ".wav";
        engine->exportAudio(buffer, filename);

        // Calculate basic stats
        float maxAmp = 0.0f;
        float meanAmp = 0.0f;

        const float* data = buffer.getReadPointer(0);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float absVal = std::abs(data[i]);
            maxAmp = std::max(maxAmp, absVal);
            meanAmp += absVal;
        }
        meanAmp /= buffer.getNumSamples();

        std::cout << "  Max amplitude: " << maxAmp << std::endl;
        std::cout << "  Mean amplitude: " << meanAmp << std::endl;
    }

    std::cout << "\nOscillator test complete!" << std::endl;
}

void runFilterTest(const TestConfig& config)
{
    std::cout << "\n=== Filter Test ===" << std::endl;

    auto engine = std::make_unique<SIDTestEngine>();

    std::cout << "Engine: " << engine->getEngineName().toStdString() << std::endl;

    // Test cutoff sweeps
    const int numCutoffs = 8;
    const int cutoffs[] = {256, 512, 768, 1024, 1280, 1536, 1792, 2047};

    for (int i = 0; i < numCutoffs; ++i)
    {
        std::cout << "\nTesting cutoff " << cutoffs[i] << "..." << std::endl;

        engine->setParameter(SIDAudioProcessor::paramWave1, 2.0f);  // Saw
        engine->setParameter(SIDAudioProcessor::paramCutoff, cutoffs[i]);
        engine->setParameter(SIDAudioProcessor::paramReso, 8.0f);
        engine->setParameter(SIDAudioProcessor::paramLP, 1.0f);
        engine->setParameter(SIDAudioProcessor::paramBP, 0.0f);
        engine->setParameter(SIDAudioProcessor::paramHP, 0.0f);
        engine->setParameter(SIDAudioProcessor::paramVol, 12.0f);

        juce::AudioSampleBuffer buffer(1, config.sampleRate * config.duration);
        buffer.clear();

        engine->generateTestTone(440.0f, config.duration, buffer, config.sampleRate);

        juce::String filename = config.outputDir + "/filter_" + juce::String(cutoffs[i]) + ".wav";
        engine->exportAudio(buffer, filename);

        // Get register values
        uint8_t reg15 = engine->getRegister(0x15);
        uint8_t reg16 = engine->getRegister(0x16);
        uint8_t reg17 = engine->getRegister(0x17);

        std::cout << "  Register 0x15: 0x" << juce::String::toHexString(reg15) << std::endl;
        std::cout << "  Register 0x16: 0x" << juce::String::toHexString(reg16) << std::endl;
        std::cout << "  Register 0x17: 0x" << juce::String::toHexString(reg17) << std::endl;
    }

    std::cout << "\nFilter test complete!" << std::endl;
}

void runEnvelopeTest(const TestConfig& config)
{
    std::cout << "\n=== Envelope Test ===" << std::endl;

    auto engine = std::make_unique<SIDTestEngine>();

    std::cout << "Engine: " << engine->getEngineName().toStdString() << std::endl;

    // Test different ADSR configurations
    struct EnvelopeConfig
    {
        const char* name;
        float a, d, s, r;
    };

    EnvelopeConfig envelopes[] = {
        {"fast", 1.0f, 2.0f, 8.0f, 2.0f},
        {"slow", 10.0f, 8.0f, 12.0f, 10.0f},
        {"pluck", 0.0f, 4.0f, 0.0f, 6.0f},
        {"pad", 8.0f, 6.0f, 12.0f, 8.0f}
    };

    for (const auto& env : envelopes)
    {
        std::cout << "\nTesting " << env.name << " envelope..." << std::endl;

        engine->setParameter(SIDAudioProcessor::paramWave1, 2.0f);
        engine->setParameter(SIDAudioProcessor::paramA1, env.a);
        engine->setParameter(SIDAudioProcessor::paramD1, env.d);
        engine->setParameter(SIDAudioProcessor::paramS1, env.s);
        engine->setParameter(SIDAudioProcessor::paramR1, env.r);
        engine->setParameter(SIDAudioProcessor::paramVol, 12.0f);

        juce::AudioSampleBuffer buffer(1, config.sampleRate * 2.0f);
        buffer.clear();

        // Create a simple note on/off sequence
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 0);
        midi.addEvent(juce::MidiMessage::noteOff(1, 60), config.sampleRate / 2);

        engine->renderToBuffer(buffer, midi);

        juce::String filename = config.outputDir + "/env_" + juce::String(env.name) + ".wav";
        engine->exportAudio(buffer, filename);
    }

    std::cout << "\nEnvelope test complete!" << std::endl;
}

void runPolyphonyTest(const TestConfig& config)
{
    std::cout << "\n=== Polyphony Test ===" << std::endl;

    auto engine = std::make_unique<SIDTestEngine>();

    std::cout << "Engine: " << engine->getEngineName().toStdString() << std::endl;

    // Test with different voice counts
    const int voiceCounts[] = {1, 2, 4, 8};

    for (int v : voiceCounts)
    {
        std::cout << "\nTesting with " << v << " voices..." << std::endl;

        engine->setParameter(SIDAudioProcessor::paramVoices, v);
        engine->setParameter(SIDAudioProcessor::paramWave1, 2.0f);
        engine->setParameter(SIDAudioProcessor::paramA1, 4.0f);
        engine->setParameter(SIDAudioProcessor::paramD1, 6.0f);
        engine->setParameter(SIDAudioProcessor::paramS1, 10.0f);
        engine->setParameter(SIDAudioProcessor::paramR1, 6.0f);
        engine->setParameter(SIDAudioProcessor::paramVol, 12.0f);

        // Generate a chord
        juce::Array<int> notes;
        for (int i = 0; i < v; ++i)
        {
            notes.add(60 + i * 3);  // C major chord arpeggio
        }

        int numSamples = config.sampleRate * config.duration;
        juce::AudioSampleBuffer buffer(1, numSamples);
        buffer.clear();

        juce::MidiBuffer midi = engine->generateMidiSequence(notes, config.duration / notes.size(), config.sampleRate);

        engine->renderToBuffer(buffer, midi);

        juce::String filename = config.outputDir + "/poly_" + juce::String(v) + "_voices.wav";
        engine->exportAudio(buffer, filename);

        std::cout << "  Active voices at peak: " << engine->getActiveVoices() << std::endl;
    }

    std::cout << "\nPolyphony test complete!" << std::endl;
}

void compareEngines(const juce::String& dirA, const juce::String& dirB)
{
    std::cout << "\n=== Engine Comparison ===" << std::endl;
    std::cout << "Directory A: " << dirA.toStdString() << std::endl;
    std::cout << "Directory B: " << dirB.toStdString() << std::endl;

    // Load and compare WAV files
    juce::WavAudioFormat wavFormat;

    // Get list of files in directory A
    juce::File dirAFile(dirA);
    juce::Array<juce::File> filesA;
    dirAFile.findChildFiles(filesA, juce::File::findFiles, false, "*.wav");

    std::cout << "\nFound " << filesA.size() << " files in directory A" << std::endl;

    for (const auto& fileA : filesA)
    {
        juce::String fileName = fileA.getFileNameWithoutExtension();

        // Try to find matching file in directory B
        juce::File fileB(dirB + "/" + fileName + ".wav");

        if (!fileB.exists())
        {
            std::cout << "  Skipping " << fileName << " - not found in directory B" << std::endl;
            continue;
        }

        // Load both files using raw pointers
        auto streamA = fileA.createInputStream();
        auto streamB = fileB.createInputStream();

        if (!streamA || !streamB)
        {
            std::cout << "  Failed to open " << fileName << std::endl;
            continue;
        }

        auto readerA = wavFormat.createReaderFor(streamA.get(), true);
        auto readerB = wavFormat.createReaderFor(streamB.get(), true);

        if (!readerA || !readerB)
        {
            std::cout << "  Failed to load " << fileName << std::endl;
            continue;
        }

        // Create buffers
        int64_t numSamples = readerA->lengthInSamples;
        juce::AudioSampleBuffer bufferA(1, (int)numSamples);
        juce::AudioSampleBuffer bufferB(1, (int)numSamples);

        readerA->read(&bufferA, 0, (int)numSamples, 0, true, true);
        readerB->read(&bufferB, 0, (int)numSamples, 0, true, true);

        // Compare
        SIDTestEngine engine;
        auto comparison = engine.compareBuffers(bufferA, bufferB);

        std::cout << "\n  File: " << fileName.toStdString() << std::endl;
        std::cout << "    Correlation: " << comparison.correlation << std::endl;
        std::cout << "    SNR: " << comparison.snr << " dB" << std::endl;
        std::cout << "    Max diff: " << comparison.maxDiff << std::endl;
        std::cout << "    Mean diff: " << comparison.meanDiff << std::endl;

        // Save comparison results
        juce::String resultFile = dirA + "/compare_" + fileName + ".json";
        engine.saveComparisonResults(comparison, "Engine A", "Engine B", resultFile);
    }

    std::cout << "\nComparison complete!" << std::endl;
}