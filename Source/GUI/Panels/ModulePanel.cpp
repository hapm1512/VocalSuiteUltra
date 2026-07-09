#include "ModulePanel.h"

ModulePanel::ModulePanel()
{
    addAndMakeVisible(powerButton);
}

void ModulePanel::configure(juce::AudioProcessorValueTreeState& state,
                            const juce::String& newTitle,
                            const juce::StringArray& knobNames,
                            const juce::StringArray& parameterIds,
                            const juce::String& powerParameterId,
                            juce::Colour newAccent,
                            const juce::String& newModeText)
{
    title = newTitle;
    labels = knobNames;
    sliderParamIds = parameterIds;
    powerParamId = powerParameterId;
    accent = newAccent;
    modeText = newModeText;

    knobs.clear(true);
    sliderAttachments.clear();
    powerAttachment.reset();

    powerButton.setButtonText("ON");
    if (state.getParameter(powerParamId) != nullptr)
        powerAttachment = std::make_unique<ButtonAttachment>(state, powerParamId, powerButton);

    const int count = juce::jmin(labels.size(), sliderParamIds.size());
    for (int i = 0; i < count; ++i)
    {
        auto* knob = new ProKnob();
        knob->setName(labels[i]);
        knobs.add(knob);
        addAndMakeVisible(knob);

        if (state.getParameter(sliderParamIds[i]) != nullptr)
            sliderAttachments.push_back(std::make_unique<SliderAttachment>(state, sliderParamIds[i], *knob));
    }

    resized();
    repaint();
}

void ModulePanel::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(3.0f);

    g.setColour(juce::Colour(0xaa000000));
    g.fillRoundedRectangle(r.translated(0.0f, 5.0f), 14.0f);

    g.setColour(juce::Colour(0xdd0d111a));
    g.fillRoundedRectangle(r, 14.0f);

    g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.24f), r.getTopLeft(),
                                           juce::Colour(0x09000000), r.getBottomRight(), false));
    g.fillRoundedRectangle(r.reduced(1.0f), 14.0f);

    g.setColour(juce::Colour(0x44ffffff));
    g.drawLine(r.getX() + 12.0f, r.getY() + 1.0f, r.getRight() - 12.0f, r.getY() + 1.0f, 1.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(r, 14.0f, 1.0f);

    auto header = getLocalBounds().withHeight(42).reduced(14, 0);
    g.setColour(accent);
    g.fillEllipse(static_cast<float>(header.getX()), static_cast<float>(header.getCentreY()) - 7.0f, 14.0f, 14.0f);

    g.setColour(juce::Colour(0xffdce8ff));
    g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    g.drawText(title, header.withTrimmedLeft(22), juce::Justification::centredLeft);

    if (modeText.isNotEmpty())
    {
        auto mode = getLocalBounds().removeFromBottom(30).reduced(18, 4);
        g.setColour(juce::Colour(0xff080b12));
        g.fillRoundedRectangle(mode.toFloat(), 7.0f);
        g.setColour(juce::Colour(0xff303b50));
        g.drawRoundedRectangle(mode.toFloat(), 7.0f, 1.0f);
        g.setColour(juce::Colour(0xff9aa7bd));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(modeText, mode, juce::Justification::centred);
    }

    if (title.containsIgnoreCase("EQ") || title.containsIgnoreCase("NOISE") || title.containsIgnoreCase("GATE"))
        drawMiniDisplay(g, getLocalBounds().reduced(15, 48).removeFromTop(80));

    drawKnobLabels(g);
}

void ModulePanel::resized()
{
    powerButton.setBounds(getWidth() - 58, 12, 42, 22);

    auto content = getLocalBounds().reduced(14, 46);
    if (modeText.isNotEmpty())
        content.removeFromBottom(32);

    if (title.containsIgnoreCase("EQ") || title.containsIgnoreCase("NOISE") || title.containsIgnoreCase("GATE"))
        content.removeFromTop(82);

    const int knobCount = knobs.size();
    if (knobCount == 0)
        return;

    const int columns = knobCount <= 2 ? knobCount : 3;
    const int rows = (knobCount + columns - 1) / columns;
    const int cellW = content.getWidth() / juce::jmax(1, columns);
    const int cellH = content.getHeight() / juce::jmax(1, rows);

    for (int i = 0; i < knobCount; ++i)
    {
        const int col = i % columns;
        const int row = i / columns;
        auto cell = juce::Rectangle<int>(content.getX() + col * cellW,
                                         content.getY() + row * cellH,
                                         cellW,
                                         cellH).reduced(5);
        knobs[i]->setBounds(cell.withSizeKeepingCentre(juce::jmin(cell.getWidth(), 82), juce::jmin(cell.getHeight(), 92)));
    }
}

void ModulePanel::drawMiniDisplay(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto r = area.toFloat();

    g.setColour(juce::Colour(0xff060910));
    g.fillRoundedRectangle(r, 8.0f);

    g.setColour(juce::Colour(0xff263044));
    g.drawRoundedRectangle(r, 8.0f, 1.0f);

    g.setColour(accent.withAlpha(0.24f));
    for (int i = 0; i < 6; ++i)
    {
        const float x = r.getX() + r.getWidth() * static_cast<float>(i) / 5.0f;
        g.drawVerticalLine(juce::roundToInt(x), r.getY(), r.getBottom());
    }

    juce::Path curve;
    for (int i = 0; i < 64; ++i)
    {
        const float p = static_cast<float>(i) / 63.0f;
        const float x = r.getX() + r.getWidth() * p;
        const float y = r.getCentreY() + std::sin(p * juce::MathConstants<float>::twoPi * 1.5f) * r.getHeight() * 0.18f
                                   - std::exp(-std::abs(p - 0.55f) * 11.0f) * r.getHeight() * 0.25f;
        if (i == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }

    g.setColour(accent);
    g.strokePath(curve, juce::PathStrokeType(1.8f));
}

void ModulePanel::drawKnobLabels(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff93a0b8));
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));

    const int count = juce::jmin(knobs.size(), labels.size());
    for (int i = 0; i < count; ++i)
    {
        auto labelArea = knobs[i]->getBounds().withHeight(16).withY(knobs[i]->getBottom() - 20);
        g.drawText(labels[i], labelArea, juce::Justification::centred);
    }
}
