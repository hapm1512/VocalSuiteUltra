#include "PluginEditor.h"

VocalSuiteUltraProAudioProcessorEditor::VocalSuiteUltraProAudioProcessorEditor(
    VocalSuiteUltraProAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1280, 760);
}

void VocalSuiteUltraProAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff090b12), r.getTopLeft(),
        juce::Colour(0xff151923), r.getBottomRight(),
        false));
    g.fillRect(r);

    g.setColour(juce::Colour(0xff66d9ff));
    g.setFont(juce::FontOptions(34.0f, juce::Font::bold));
    g.drawText("VOCAL SUITE ULTRA PRO", getLocalBounds().withHeight(80),
               juce::Justification::centred);

    g.setColour(juce::Colour(0xffaab4c8));
    g.setFont(juce::FontOptions(16.0f));
    g.drawText("Epic 1 Core Framework - JUCE 8 - Hai Pham",
               getLocalBounds().withTrimmedTop(80).withHeight(40),
               juce::Justification::centred);
}

void VocalSuiteUltraProAudioProcessorEditor::resized()
{
}

