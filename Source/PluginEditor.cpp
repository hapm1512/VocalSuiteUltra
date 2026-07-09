#include "PluginEditor.h"

VocalSuiteUltraProAudioProcessorEditor::VocalSuiteUltraProAudioProcessorEditor(
    VocalSuiteUltraProAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1536, 864);
    setResizable(true, true);

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
    drawHeader(g);

    auto area = bodyArea.reduced(16);
    auto top = area.removeFromTop(350);
    auto mid = area.removeFromTop(342);
    auto bottom = area;

    auto input = top.removeFromLeft(100);
    drawGlassPanel(g, input, "INPUT");

    drawGlassPanel(g, top.removeFromLeft(230).reduced(7, 0), "CORRECTION");
    drawGlassPanel(g, top.removeFromLeft(250).reduced(7, 0), "NOISE REDUCTION");
    drawGlassPanel(g, top.removeFromLeft(250).reduced(7, 0), "PREAMP");
    drawGlassPanel(g, top.removeFromLeft(250).reduced(7, 0), "GATE");
    drawGlassPanel(g, top.removeFromLeft(250).reduced(7, 0), "HIGH PASS");

    auto output = top.reduced(7, 0);
    drawGlassPanel(g, output, "OUTPUT");

    drawGlassPanel(g, mid.removeFromLeft(300), "SURGICAL EQ");
    drawGlassPanel(g, mid.removeFromLeft(250).reduced(7, 0), "DE-ESSER");
    drawGlassPanel(g, mid.removeFromLeft(280).reduced(7, 0), "COMPRESSOR 1176");
    drawGlassPanel(g, mid.removeFromLeft(250).reduced(7, 0), "TONE EQ");
    drawGlassPanel(g, mid.removeFromLeft(210).reduced(7, 0), "SATURATION");
    drawGlassPanel(g, mid.reduced(7, 0), "HI-END EXCITER");

    drawGlassPanel(g, bottom.removeFromLeft(130), "MIX");
    drawGlassPanel(g, bottom.removeFromLeft(130).reduced(7, 0), "WIDTH");
    drawGlassPanel(g, bottom.removeFromLeft(440).reduced(7, 0), "SPECTRUM ANALYZER");
    drawGlassPanel(g, bottom.removeFromLeft(170).reduced(7, 0), "LEVELS");
    drawGlassPanel(g, bottom.removeFromLeft(160).reduced(7, 0), "LOUDNESS");
    drawGlassPanel(g, bottom.removeFromLeft(180).reduced(7, 0), "CORRELATION");
    drawGlassPanel(g, bottom.reduced(7, 0), "PRESETS");

    drawFooter(g);
}

void VocalSuiteUltraProAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();

    headerArea = r.removeFromTop(74);
    footerArea = r.removeFromBottom(44);
    bodyArea = r;

    auto area = bodyArea.reduced(16);
    auto top = area.removeFromTop(350);
    auto mid = area.removeFromTop(342);
    auto bottom = area;

    auto input = top.removeFromLeft(100);
    inputMeter.setBounds(input.reduced(25, 56));

    top.removeFromLeft(230);
    top.removeFromLeft(250);
    top.removeFromLeft(250);
    top.removeFromLeft(250);
    top.removeFromLeft(250);

    auto output = top;
    outputMeter.setBounds(output.reduced(25, 56));

    mid.removeFromLeft(300);
    mid.removeFromLeft(250);
    mid.removeFromLeft(280);
    mid.removeFromLeft(250);
    mid.removeFromLeft(210);

    bottom.removeFromLeft(130);
    bottom.removeFromLeft(130);

    auto spectrum = bottom.removeFromLeft(440).reduced(14, 42);
    spectrumAnalyzer.setBounds(spectrum);

    auto levels = bottom.removeFromLeft(170).reduced(28, 42);
    levelMeter.setBounds(levels);

    auto lufs = bottom.removeFromLeft(160).reduced(14, 42);
    lufsMeter.setBounds(lufs);

    auto corr = bottom.removeFromLeft(180).reduced(14, 42);
    correlationMeter.setBounds(corr);
}

void VocalSuiteUltraProAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff05070d), r.getTopLeft(),
        juce::Colour(0xff151927), r.getBottomRight(),
        false));

    g.fillRect(r);

    g.setColour(juce::Colour(0x2200ccff));
    g.drawRect(getLocalBounds(), 1);
}

void VocalSuiteUltraProAudioProcessorEditor::drawHeader(juce::Graphics& g)
{
    auto r = headerArea.reduced(14, 10);

    g.setColour(juce::Colour(0xee0b0f17));
    g.fillRoundedRectangle(r.toFloat(), 14.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r.toFloat(), 14.0f, 1.0f);

    auto left = r.removeFromLeft(350);

    g.setColour(juce::Colour(0xffe8eefc));
    g.setFont(juce::FontOptions(23.0f, juce::Font::bold));
    g.drawText("VOCAL SUITE", left.removeFromTop(28), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff7f8ca6));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("PROFESSIONAL  V1.0.0", left, juce::Justification::centredLeft);

    auto preset = headerArea.withSizeKeepingCentre(380, 42);

    g.setColour(juce::Colour(0xff090c13));
    g.fillRoundedRectangle(preset.toFloat(), 10.0f);

    g.setColour(juce::Colour(0xff4c5a78));
    g.drawRoundedRectangle(preset.toFloat(), 10.0f, 1.0f);

    g.setColour(juce::Colour(0xfff3f7ff));
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("VOCAL - STUDIO PRO", preset, juce::Justification::centred);

    auto right = headerArea.removeFromRight(430).reduced(14, 16);

    const char* labels[] = { "SAVE", "DEL", "A", "B", "A/B", "SET", "PWR" };

    for (auto label : labels)
    {
        auto b = right.removeFromLeft(58).reduced(4, 0);

        g.setColour(juce::Colour(0xff101522));
        g.fillRoundedRectangle(b.toFloat(), 8.0f);

        g.setColour(juce::Colour(0xff2d8cff));
        g.drawRoundedRectangle(b.toFloat(), 8.0f, 1.1f);

        g.setColour(juce::Colour(0xffdbe9ff));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(label, b, juce::Justification::centred);
    }
}

void VocalSuiteUltraProAudioProcessorEditor::drawFooter(juce::Graphics& g)
{
    auto r = footerArea.reduced(14, 6);

    g.setColour(juce::Colour(0xee0a0d14));
    g.fillRoundedRectangle(r.toFloat(), 10.0f);

    g.setColour(juce::Colour(0xff59657d));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("QUALITY: HIGH     48 kHz     LATENCY: 0.0 ms",
               r.removeFromLeft(420), juce::Justification::centredLeft);

    g.drawText("CPU: 0.0%     COPYRIGHT HAI PHAM",
               r, juce::Justification::centredRight);
}

void VocalSuiteUltraProAudioProcessorEditor::drawGlassPanel(
    juce::Graphics& g,
    juce::Rectangle<int> area,
    juce::String title)
{
    if (area.getWidth() < 40 || area.getHeight() < 40)
        return;

    auto r = area.reduced(4).toFloat();

    g.setColour(juce::Colour(0xaa000000));
    g.fillRoundedRectangle(r.translated(0.0f, 5.0f), 14.0f);

    g.setColour(juce::Colour(0xdd0d111a));
    g.fillRoundedRectangle(r, 14.0f);

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0x5534d6ff), r.getTopLeft(),
        juce::Colour(0x11000000), r.getBottomRight(),
        false));
    g.fillRoundedRectangle(r.reduced(1.0f), 14.0f);

    g.setColour(juce::Colour(0x55ffffff));
    g.drawLine(r.getX() + 12.0f, r.getY() + 1.0f,
               r.getRight() - 12.0f, r.getY() + 1.0f, 1.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r, 14.0f, 1.0f);

    g.setColour(juce::Colour(0xffdce8ff));
    g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    g.drawText(title, area.withHeight(42).reduced(14, 0),
               juce::Justification::centredLeft);

    auto power = juce::Rectangle<int>(area.getRight() - 58, area.getY() + 12, 42, 22);

    g.setColour(juce::Colour(0xff103b22));
    g.fillRoundedRectangle(power.toFloat(), 6.0f);

    g.setColour(juce::Colour(0xff43ff88));
    g.drawRoundedRectangle(power.toFloat(), 6.0f, 1.0f);

    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("ON", power, juce::Justification::centred);
}
