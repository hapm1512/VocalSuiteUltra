#include "PluginEditor.h"

VocalSuiteUltraProAudioProcessorEditor::VocalSuiteUltraProAudioProcessorEditor(
    VocalSuiteUltraProAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    juce::ignoreUnused(processor);

    configurePanels();
    addAllComponents();

    setSize(1536, 864);
    setResizeLimits(1200, 720, 2200, 1400);
    setResizable(true, true);
}

void VocalSuiteUltraProAudioProcessorEditor::configurePanels()
{
    auto makeNames = [](std::initializer_list<const char*> items)
    {
        juce::StringArray names;
        for (auto* item : items)
            names.add(item);
        return names;
    };

    correctionPanel.configure("CORRECTION", makeNames({ "SPEED", "MIX" }), juce::Colour(0xff2d8cff), "KEY: D#  |  +3.2 CENT");
    noisePanel.configure("NOISE REDUCTION", makeNames({ "REDUCE", "DETAIL" }), juce::Colour(0xff54d941), "AI ADAPTIVE");
    preampPanel.configure("PREAMP", makeNames({ "DRIVE", "OUTPUT" }), juce::Colour(0xffffa326), "MODE: TUBE");
    gatePanel.configure("GATE", makeNames({ "THRESH", "RANGE", "ATTACK", "RELEASE" }), juce::Colour(0xffc06bff));
    highPassPanel.configure("HIGH PASS", makeNames({ "FREQ", "SLOPE" }), juce::Colour(0xff34d6ff), "80 Hz  |  12 dB/OCT");

    surgicalEqPanel.configure("SURGICAL EQ", makeNames({ "FREQ", "GAIN", "Q" }), juce::Colour(0xffffd744));
    deesserPanel.configure("DE-ESSER", makeNames({ "THRESH", "REDUCE" }), juce::Colour(0xffff72c8), "5.0 kHz - 8.0 kHz");
    compressorPanel.configure("COMPRESSOR 1176", makeNames({ "ATTACK", "RELEASE", "INPUT", "OUTPUT", "MIX" }), juce::Colour(0xff62a8ff), "RATIO: 4:1");
    toneEqPanel.configure("TONE EQ", makeNames({ "LOW", "MID", "HIGH", "AIR", "GAIN" }), juce::Colour(0xff80ee66));
    saturationPanel.configure("SATURATION", makeNames({ "DRIVE", "MIX" }), juce::Colour(0xffffa326), "TYPE: TUBE");
    hiEndPanel.configure("HI-END EXCITER", makeNames({ "FREQ", "AMOUNT" }), juce::Colour(0xff34d6ff), "12.0 kHz");

    mixPanel.configure("IN / OUT", makeNames({ "MIX" }), juce::Colour(0xff8db7ff));
    widthPanel.configure("WIDTH", makeNames({ "WIDTH" }), juce::Colour(0xffb98cff), "MONO BASS: ON");
}

void VocalSuiteUltraProAudioProcessorEditor::addAllComponents()
{
    addAndMakeVisible(headerBar);
    addAndMakeVisible(footerBar);

    for (auto* panel : { &inputPanel, &outputPanel, &spectrumPanel, &levelsPanel, &lufsPanel, &correlationPanel, &presetPanel })
        addAndMakeVisible(panel);

    for (auto* panel : { &correctionPanel, &noisePanel, &preampPanel, &gatePanel, &highPassPanel,
                         &surgicalEqPanel, &deesserPanel, &compressorPanel, &toneEqPanel,
                         &saturationPanel, &hiEndPanel, &mixPanel, &widthPanel })
        addAndMakeVisible(panel);

    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(levelMeter);
    addAndMakeVisible(lufsMeter);
    addAndMakeVisible(correlationMeter);
    addAndMakeVisible(spectrumAnalyzer);
}

void VocalSuiteUltraProAudioProcessorEditor::paint(juce::Graphics& g)
{
    drawBackground(g);
    drawPresetList(g);
}

void VocalSuiteUltraProAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();

    headerArea = r.removeFromTop(74);
    footerArea = r.removeFromBottom(44);
    bodyArea = r.reduced(16);

    headerBar.setBounds(headerArea);
    footerBar.setBounds(footerArea);

    auto area = bodyArea;
    auto top = area.removeFromTop(350);
    auto mid = area.removeFromTop(342);
    auto bottom = area;

    inputPanel.setBounds(top.removeFromLeft(100));
    inputMeter.setBounds(inputPanel.getBounds().reduced(25, 56));

    correctionPanel.setBounds(top.removeFromLeft(230).reduced(7, 0));
    noisePanel.setBounds(top.removeFromLeft(250).reduced(7, 0));
    preampPanel.setBounds(top.removeFromLeft(250).reduced(7, 0));
    gatePanel.setBounds(top.removeFromLeft(250).reduced(7, 0));
    highPassPanel.setBounds(top.removeFromLeft(250).reduced(7, 0));

    outputPanel.setBounds(top.reduced(7, 0));
    outputMeter.setBounds(outputPanel.getBounds().reduced(25, 56));

    surgicalEqPanel.setBounds(mid.removeFromLeft(300));
    deesserPanel.setBounds(mid.removeFromLeft(250).reduced(7, 0));
    compressorPanel.setBounds(mid.removeFromLeft(280).reduced(7, 0));
    toneEqPanel.setBounds(mid.removeFromLeft(250).reduced(7, 0));
    saturationPanel.setBounds(mid.removeFromLeft(210).reduced(7, 0));
    hiEndPanel.setBounds(mid.reduced(7, 0));

    mixPanel.setBounds(bottom.removeFromLeft(130));
    widthPanel.setBounds(bottom.removeFromLeft(130).reduced(7, 0));

    spectrumPanel.setBounds(bottom.removeFromLeft(440).reduced(7, 0));
    spectrumAnalyzer.setBounds(spectrumPanel.getBounds().reduced(14, 42));

    levelsPanel.setBounds(bottom.removeFromLeft(170).reduced(7, 0));
    levelMeter.setBounds(levelsPanel.getBounds().reduced(28, 42));

    lufsPanel.setBounds(bottom.removeFromLeft(160).reduced(7, 0));
    lufsMeter.setBounds(lufsPanel.getBounds().reduced(14, 42));

    correlationPanel.setBounds(bottom.removeFromLeft(180).reduced(7, 0));
    correlationMeter.setBounds(correlationPanel.getBounds().reduced(14, 42));

    presetPanel.setBounds(bottom.reduced(7, 0));
}

void VocalSuiteUltraProAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff05070d), r.getTopLeft(),
                                           juce::Colour(0xff151927), r.getBottomRight(), false));
    g.fillRect(r);

    g.setColour(juce::Colour(0x141f8cff));
    for (int i = 0; i < getWidth(); i += 64)
        g.drawVerticalLine(i, 0.0f, static_cast<float>(getHeight()));

    for (int y = 0; y < getHeight(); y += 64)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));

    g.setColour(juce::Colour(0x2200ccff));
    g.drawRect(getLocalBounds(), 1);
}

void VocalSuiteUltraProAudioProcessorEditor::drawPresetList(juce::Graphics& g)
{
    if (presetPanel.getBounds().isEmpty())
        return;

    auto list = presetPanel.getBounds().reduced(18, 46);

    const juce::String presets[] =
    {
        "VOCAL - STUDIO PRO",
        "VOCAL - MALE CLEAR",
        "VOCAL - FEMALE BRIGHT",
        "VOCAL - RAP / TRAP",
        "VOCAL - BALLAD",
        "VOCAL - LIVE STAGE"
    };

    for (int i = 0; i < 6; ++i)
    {
        auto row = list.removeFromTop(22);
        const bool selected = i == 0;

        if (selected)
        {
            g.setColour(juce::Colour(0xff30245c));
            g.fillRoundedRectangle(row.toFloat(), 5.0f);
        }

        g.setColour(selected ? juce::Colour(0xffd9ccff) : juce::Colour(0xff8d98ad));
        g.setFont(juce::FontOptions(12.0f, selected ? juce::Font::bold : juce::Font::plain));
        g.drawText(presets[i], row.reduced(8, 0), juce::Justification::centredLeft);
    }
}
