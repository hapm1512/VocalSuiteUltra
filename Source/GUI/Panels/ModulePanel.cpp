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
    const bool compact = getWidth() < 180 || getHeight() < 150;
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

    const int headerHeight = compact ? 36 : 42;
    auto header = getLocalBounds().withHeight(headerHeight).reduced(compact ? 10 : 14, 0);

    const float dotSize = compact ? 11.0f : 14.0f;
    g.setColour(accent);
    g.fillEllipse(static_cast<float>(header.getX()),
                  static_cast<float>(header.getCentreY()) - dotSize * 0.5f,
                  dotSize,
                  dotSize);

    auto titleArea = header.withTrimmedLeft(compact ? 17 : 22)
                           .withTrimmedRight(compact ? 46 : 50);

    g.setColour(juce::Colour(0xffdce8ff));
    g.setFont(juce::FontOptions(compact ? 11.5f : 13.5f, juce::Font::bold));
    g.drawFittedText(title, titleArea, juce::Justification::centredLeft, 1, 0.85f);

    if (modeText.isNotEmpty())
    {
        auto mode = getLocalBounds().removeFromBottom(compact ? 25 : 30)
                                    .reduced(compact ? 10 : 18, compact ? 3 : 4);
        g.setColour(juce::Colour(0xff080b12));
        g.fillRoundedRectangle(mode.toFloat(), 7.0f);
        g.setColour(juce::Colour(0xff303b50));
        g.drawRoundedRectangle(mode.toFloat(), 7.0f, 1.0f);
        g.setColour(juce::Colour(0xff9aa7bd));
        g.setFont(juce::FontOptions(compact ? 9.0f : 11.0f, juce::Font::bold));
        g.drawFittedText(modeText, mode.reduced(3, 0), juce::Justification::centred, 1, 0.75f);
    }

    if (! compact && (title.containsIgnoreCase("EQ")
                    || title.containsIgnoreCase("NOISE")
                    || title.containsIgnoreCase("GATE")))
    {
        drawMiniDisplay(g, getLocalBounds().reduced(15, 48).removeFromTop(80));
    }

    drawKnobLabels(g);
}

void ModulePanel::resized()
{
    const bool compact = getWidth() < 180 || getHeight() < 150;

    if (compact)
        powerButton.setBounds(getWidth() - 49, 9, 38, 20);
    else
        powerButton.setBounds(getWidth() - 58, 12, 42, 22);

    const int headerHeight = compact ? 36 : 46;
    auto content = getLocalBounds().reduced(compact ? 8 : 14, 0);
    content.removeFromTop(headerHeight);
    content.removeFromBottom(compact ? 6 : 8);

    if (modeText.isNotEmpty())
        content.removeFromBottom(compact ? 27 : 32);

    if (! compact && (title.containsIgnoreCase("EQ")
                    || title.containsIgnoreCase("NOISE")
                    || title.containsIgnoreCase("GATE")))
    {
        content.removeFromTop(82);
    }

    const int knobCount = knobs.size();
    if (knobCount == 0 || content.isEmpty())
        return;

    const int columns = compact ? knobCount : (knobCount <= 2 ? knobCount : 3);
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
                                         cellH).reduced(compact ? 2 : 5);

        // Reserve a dedicated strip above the knob for its label.
        cell.removeFromTop(compact ? 13 : 16);

        const int maxW = compact ? 64 : 82;
        const int maxH = compact ? 70 : 92;
        knobs[i]->setBounds(cell.withSizeKeepingCentre(juce::jmin(cell.getWidth(), maxW),
                                                       juce::jmin(cell.getHeight(), maxH)));
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
    const bool compact = getWidth() < 180 || getHeight() < 150;

    g.setColour(juce::Colour(0xff93a0b8));
    g.setFont(juce::FontOptions(compact ? 9.0f : 10.5f, juce::Font::bold));

    const int count = juce::jmin(knobs.size(), labels.size());
    for (int i = 0; i < count; ++i)
    {
        const auto knobBounds = knobs[i]->getBounds();
        auto labelArea = juce::Rectangle<int>(knobBounds.getX() - 4,
                                              knobBounds.getY() - (compact ? 13 : 16),
                                              knobBounds.getWidth() + 8,
                                              compact ? 13 : 16);
        g.drawFittedText(labels[i], labelArea, juce::Justification::centred, 1, 0.8f);
    }
}
