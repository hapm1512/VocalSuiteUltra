#include "PluginEditor.h"

VocalSuiteUltraProAudioProcessorEditor::VocalSuiteUltraProAudioProcessorEditor(
    VocalSuiteUltraProAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1536, 864);
    setResizable(true, true);
}

void VocalSuiteUltraProAudioProcessorEditor::paint(juce::Graphics& g)
{
    drawBackground(g);
    drawHeader(g);
    drawFooter(g);

    auto area = bodyArea.reduced(18);

    auto top = area.removeFromTop(350);
    auto bottom = area;

    drawGlassPanel(g, top.removeFromLeft(180), "INPUT");
    drawGlassPanel(g, top.removeFromLeft(240).reduced(8, 0), "CORRECTION");
    drawGlassPanel(g, top.removeFromLeft(240).reduced(8, 0), "NOISE REDUCTION");
    drawGlassPanel(g, top.removeFromLeft(240).reduced(8, 0), "PREAMP");
    drawGlassPanel(g, top.removeFromLeft(240).reduced(8, 0), "GATE");
    drawGlassPanel(g, top.removeFromLeft(240).reduced(8, 0), "HIGH PASS");
    drawGlassPanel(g, top.reduced(8, 0), "OUTPUT");

    drawGlassPanel(g, bottom.removeFromLeft(300), "SURGICAL EQ");
    drawGlassPanel(g, bottom.removeFromLeft(250).reduced(8, 0), "DE-ESSER");
    drawGlassPanel(g, bottom.removeFromLeft(280).reduced(8, 0), "COMPRESSOR 1176");
    drawGlassPanel(g, bottom.removeFromLeft(250).reduced(8, 0), "TONE EQ");
    drawGlassPanel(g, bottom.removeFromLeft(210).reduced(8, 0), "SATURATION");
    drawGlassPanel(g, bottom.reduced(8, 0), "HI-END EXCITER");
}

void VocalSuiteUltraProAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();

    headerArea = r.removeFromTop(74);
    footerArea = r.removeFromBottom(72);
    bodyArea = r;
}

void VocalSuiteUltraProAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff06080e), r.getTopLeft(),
        juce::Colour(0xff151927), r.getBottomRight(),
        false));

    g.fillRect(r);

    g.setColour(juce::Colour(0x2200ccff));
    g.drawRect(getLocalBounds(), 1);
}

void VocalSuiteUltraProAudioProcessorEditor::drawHeader(juce::Graphics& g)
{
    auto r = headerArea.reduced(14, 10);

    g.setColour(juce::Colour(0xdd0c1018));
    g.fillRoundedRectangle(r.toFloat(), 14.0f);

    g.setColour(juce::Colour(0x55334466));
    g.drawRoundedRectangle(r.toFloat(), 14.0f, 1.0f);

    g.setColour(juce::Colour(0xffe8eefc));
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("VOCAL SUITE", r.removeFromLeft(230), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff7f8ca6));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("PROFESSIONAL  •  V1.0.0", 88, 42, 260, 20, juce::Justification::centredLeft);

    auto preset = headerArea.withSizeKeepingCentre(360, 42);

    g.setColour(juce::Colour(0xff090c13));
    g.fillRoundedRectangle(preset.toFloat(), 10.0f);

    g.setColour(juce::Colour(0xff4c5a78));
    g.drawRoundedRectangle(preset.toFloat(), 10.0f, 1.0f);

    g.setColour(juce::Colour(0xfff3f7ff));
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("VOCAL - STUDIO PRO", preset, juce::Justification::centred);

    auto right = headerArea.removeFromRight(360).reduced(14, 16);

    const char* labels[] = { "A", "B", "A↔B", "⚙", "?", "⏻" };

    for (auto label : labels)
    {
        auto b = right.removeFromLeft(48).reduced(4, 0);

        g.setColour(juce::Colour(0xff101522));
        g.fillRoundedRectangle(b.toFloat(), 8.0f);

        g.setColour(juce::Colour(0xff2d8cff));
        g.drawRoundedRectangle(b.toFloat(), 8.0f, 1.2f);

        g.setColour(juce::Colour(0xffdbe9ff));
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        g.drawText(label, b, juce::Justification::centred);
    }
}

void VocalSuiteUltraProAudioProcessorEditor::drawFooter(juce::Graphics& g)
{
    auto r = footerArea.reduced(14, 8);

    g.setColour(juce::Colour(0xdd0a0d14));
    g.fillRoundedRectangle(r.toFloat(), 12.0f);

    g.setColour(juce::Colour(0xff59657d));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("QUALITY: HIGH", r.removeFromLeft(220), juce::Justification::centredLeft);

    g.drawText("CPU: 0.0%     COPYRIGHT HAI PHAM", r, juce::Justification::centredRight);
}

void VocalSuiteUltraProAudioProcessorEditor::drawGlassPanel(
    juce::Graphics& g,
    juce::Rectangle<int> area,
    juce::String title)
{
    if (area.getWidth() < 40 || area.getHeight() < 40)
        return;

    auto r = area.reduced(4).toFloat();

    g.setColour(juce::Colour(0xcc0d111a));
    g.fillRoundedRectangle(r, 12.0f);

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0x4434d6ff), r.getTopLeft(),
        juce::Colour(0x11000000), r.getBottomRight(),
        false));
    g.fillRoundedRectangle(r.reduced(1.0f), 12.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r, 12.0f, 1.0f);

    g.setColour(juce::Colour(0xffdce8ff));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(title, area.removeFromTop(42).reduced(14, 0), juce::Justification::centredLeft);

    auto power = juce::Rectangle<int>(area.getRight() - 58, area.getY() + 12, 42, 22);

    g.setColour(juce::Colour(0xff103b22));
    g.fillRoundedRectangle(power.toFloat(), 6.0f);

    g.setColour(juce::Colour(0xff43ff88));
    g.drawRoundedRectangle(power.toFloat(), 6.0f, 1.0f);

    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("ON", power, juce::Justification::centred);
}
