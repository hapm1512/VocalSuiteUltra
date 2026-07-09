#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <functional>

class SpectrumAnalyzer final : public juce::Component,
                               private juce::Timer
{
public:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    using AudioSnapshot = std::array<float, fftSize>;
    using SampleProvider = std::function<bool(AudioSnapshot&)>;

    SpectrumAnalyzer();

    void setSampleProvider(SampleProvider provider);
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    void updateSpectrum();
    float frequencyToX(float frequency, juce::Rectangle<float> graph) const noexcept;
    float magnitudeToY(float magnitudeDb, juce::Rectangle<float> graph) const noexcept;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };

    AudioSnapshot audioSnapshot {};
    std::array<float, fftSize * 2> fftData {};
    std::array<float, 192> displayMagnitudes {};
    std::array<float, 192> peakHold {};
    SampleProvider sampleProvider;

    juce::Path spectrumPath;
    juce::Path peakPath;
    bool hasSignal = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};
