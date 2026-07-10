#include "ModulePanel.h"

#include <cmath>

namespace
{
    float frequencyToX(float frequency, const juce::Rectangle<float>& area)
    {
        const float minFrequency = 20.0f;
        const float maxFrequency = 20000.0f;
        const float clamped = juce::jlimit(minFrequency, maxFrequency, frequency);
        const float proportion = std::log(clamped / minFrequency) / std::log(maxFrequency / minFrequency);
        return area.getX() + area.getWidth() * proportion;
    }

    float gainToY(float gainDb, const juce::Rectangle<float>& area, float rangeDb = 18.0f)
    {
        return juce::jmap(juce::jlimit(-rangeDb, rangeDb, gainDb),
                          -rangeDb, rangeDb,
                          area.getBottom(), area.getY());
    }

    float gaussian(float x, float centre, float width)
    {
        const float safeWidth = juce::jmax(0.01f, width);
        const float distance = (x - centre) / safeWidth;
        return std::exp(-0.5f * distance * distance);
    }
}

ModulePanel::ModulePanel()
{
    addAndMakeVisible(powerButton);
    startTimerHz(30);
}

ModulePanel::~ModulePanel()
{
    stopTimer();
    detachParameterListeners();
}

void ModulePanel::configure(juce::AudioProcessorValueTreeState& state,
                            const juce::String& newTitle,
                            const juce::StringArray& knobNames,
                            const juce::StringArray& parameterIds,
                            const juce::String& powerParameterId,
                            juce::Colour newAccent,
                            const juce::String& newModeText)
{
    detachParameterListeners();

    title = newTitle;
    labels = knobNames;
    sliderParamIds = parameterIds;
    powerParamId = powerParameterId;
    accent = newAccent;
    modeText = newModeText;
    parameterState = &state;

    knobs.clear(true);
    sliderAttachments.clear();
    powerAttachment.reset();

    powerButton.setButtonText("ON");
    if (state.getParameter(powerParamId) != nullptr)
    {
        state.addParameterListener(powerParamId, this);
        powerAttachment = std::make_unique<ButtonAttachment>(state, powerParamId, powerButton);
    }

    const int count = juce::jmin(labels.size(), sliderParamIds.size());
    for (int i = 0; i < count; ++i)
    {
        auto* knob = new ProKnob();
        knob->setName(labels[i]);
        knobs.add(knob);
        addAndMakeVisible(knob);

        if (state.getParameter(sliderParamIds[i]) != nullptr)
        {
            state.addParameterListener(sliderParamIds[i], this);
            sliderAttachments.push_back(std::make_unique<SliderAttachment>(state, sliderParamIds[i], *knob));
        }
    }

    graphDirty.store(true, std::memory_order_release);
    resized();
    repaint();
}

void ModulePanel::parameterChanged(const juce::String&, float)
{
    graphDirty.store(true, std::memory_order_release);
}

void ModulePanel::timerCallback()
{
    if (graphDirty.exchange(false, std::memory_order_acq_rel))
        repaint();
}

void ModulePanel::detachParameterListeners()
{
    if (parameterState == nullptr)
        return;

    if (powerParamId.isNotEmpty() && parameterState->getParameter(powerParamId) != nullptr)
        parameterState->removeParameterListener(powerParamId, this);

    for (const auto& parameterId : sliderParamIds)
        if (parameterState->getParameter(parameterId) != nullptr)
            parameterState->removeParameterListener(parameterId, this);

    parameterState = nullptr;
}

bool ModulePanel::hasInteractiveGraph() const noexcept
{
    return title.containsIgnoreCase("NOISE")
        || title.containsIgnoreCase("GATE")
        || title.containsIgnoreCase("SURGICAL EQ")
        || title.containsIgnoreCase("TONE EQ");
}

float ModulePanel::getParameterValue(const juce::String& parameterId, float fallback) const noexcept
{
    if (parameterState == nullptr)
        return fallback;

    if (const auto* value = parameterState->getRawParameterValue(parameterId))
        return value->load(std::memory_order_relaxed);

    return fallback;
}

float ModulePanel::getNormalisedParameterValue(const juce::String& parameterId, float fallback) const noexcept
{
    if (parameterState == nullptr)
        return fallback;

    if (const auto* parameter = parameterState->getParameter(parameterId))
        return parameter->getValue();

    return fallback;
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

    if (!compact && hasInteractiveGraph())
        drawMiniDisplay(g, getLocalBounds().reduced(15, 48).removeFromTop(80));

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

    if (!compact && hasInteractiveGraph())
        content.removeFromTop(82);

    const int knobCount = knobs.size();
    if (knobCount == 0 || content.isEmpty())
        return;

    const int columns = compact ? knobCount : (knobCount <= 2 ? knobCount : 3);
    const int rows = (knobCount + columns - 1) / columns;
    const int cellW = content.getWidth() / juce::jmax(1, columns);
    const int cellH = content.getHeight() / juce::jmax(1, rows);

    for (int i = 0; i < knobCount; ++i)
    {
        int col = i % columns;
        const int row = i / columns;

        // Professional visual balance:
        // 4 controls = 3 on top, 1 centred below.
        // 5 controls = 3 on top, 2 at the outer sides below.
        if (!compact && row == 1 && knobCount == 4)
            col = 1;
        else if (!compact && row == 1 && knobCount == 5)
            col = (i == 3 ? 0 : 2);

        auto cell = juce::Rectangle<int>(content.getX() + col * cellW,
                                         content.getY() + row * cellH,
                                         cellW,
                                         cellH).reduced(compact ? 2 : 5);

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

    auto graphArea = r.reduced(7.0f, 6.0f);

    g.setColour(accent.withAlpha(0.13f));
    for (int i = 0; i < 7; ++i)
    {
        const float x = graphArea.getX() + graphArea.getWidth() * static_cast<float>(i) / 6.0f;
        g.drawVerticalLine(juce::roundToInt(x), graphArea.getY(), graphArea.getBottom());
    }

    for (int i = 0; i < 5; ++i)
    {
        const float y = graphArea.getY() + graphArea.getHeight() * static_cast<float>(i) / 4.0f;
        g.drawHorizontalLine(juce::roundToInt(y), graphArea.getX(), graphArea.getRight());
    }

    if (title.containsIgnoreCase("NOISE"))
        drawNoiseGraph(g, graphArea);
    else if (title.containsIgnoreCase("GATE"))
        drawGateGraph(g, graphArea);
    else if (title.containsIgnoreCase("SURGICAL EQ"))
        drawSurgicalEqGraph(g, graphArea);
    else if (title.containsIgnoreCase("TONE EQ"))
        drawToneEqGraph(g, graphArea);
}

void ModulePanel::drawNoiseGraph(juce::Graphics& g, juce::Rectangle<float> area)
{
    const float reduction = getNormalisedParameterValue("NOISE_REDUCE", 0.5f);
    const float detail = getNormalisedParameterValue("NOISE_DETAIL", 0.5f);
    const bool enabled = getParameterValue("NOISE_ON", 1.0f) >= 0.5f;

    const float floorY = juce::jmap(reduction, 0.0f, 1.0f,
                                    area.getBottom() - area.getHeight() * 0.18f,
                                    area.getY() + area.getHeight() * 0.62f);

    g.setColour(accent.withAlpha(enabled ? 0.16f : 0.07f));
    g.fillRect(juce::Rectangle<float>(area.getX(), floorY,
                                     area.getWidth(), area.getBottom() - floorY));

    juce::Path curve;
    for (int i = 0; i < 96; ++i)
    {
        const float p = static_cast<float>(i) / 95.0f;
        const float texture = std::sin(p * 31.0f) * (0.02f + detail * 0.05f)
                            + std::sin(p * 73.0f) * 0.012f;
        const float envelope = 0.38f + 0.32f * std::exp(-p * (1.4f + detail * 2.2f));
        const float attenuation = reduction * (0.20f + 0.48f * (1.0f - p));
        const float value = juce::jlimit(0.04f, 0.95f, envelope + texture - attenuation);
        const float x = area.getX() + area.getWidth() * p;
        const float y = area.getBottom() - area.getHeight() * value;

        if (i == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }

    g.setColour((enabled ? accent : juce::Colours::grey).withAlpha(0.92f));
    g.strokePath(curve, juce::PathStrokeType(1.8f));

    g.setColour(accent.withAlpha(0.65f));
    g.drawHorizontalLine(juce::roundToInt(floorY), area.getX(), area.getRight());
}

void ModulePanel::drawGateGraph(juce::Graphics& g, juce::Rectangle<float> area)
{
    const float threshold = getNormalisedParameterValue("GATE_THRESH", 0.5f);
    const float range = getNormalisedParameterValue("GATE_RANGE", 0.5f);
    const float attack = getNormalisedParameterValue("GATE_ATTACK", 0.3f);
    const float release = getNormalisedParameterValue("GATE_RELEASE", 0.5f);
    const bool enabled = getParameterValue("GATE_ON", 1.0f) >= 0.5f;

    const float thresholdX = area.getX() + area.getWidth() * threshold;
    const float closedY = area.getBottom() - area.getHeight() * (0.06f + (1.0f - range) * 0.18f);
    const float kneeWidth = area.getWidth() * (0.05f + release * 0.16f);
    const float curvePower = 1.2f + attack * 3.0f;

    juce::Path curve;
    for (int i = 0; i < 96; ++i)
    {
        const float p = static_cast<float>(i) / 95.0f;
        const float x = area.getX() + area.getWidth() * p;
        float openAmount = juce::jlimit(0.0f, 1.0f,
                                       (x - (thresholdX - kneeWidth * 0.5f)) / juce::jmax(1.0f, kneeWidth));
        openAmount = std::pow(openAmount, curvePower);
        const float openY = area.getBottom() - area.getHeight() * p;
        const float y = juce::jmap(openAmount, closedY, openY);

        if (i == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }

    g.setColour(accent.withAlpha(0.34f));
    g.drawVerticalLine(juce::roundToInt(thresholdX), area.getY(), area.getBottom());

    g.setColour((enabled ? accent : juce::Colours::grey).withAlpha(0.92f));
    g.strokePath(curve, juce::PathStrokeType(1.9f));
}

void ModulePanel::drawSurgicalEqGraph(juce::Graphics& g, juce::Rectangle<float> area)
{
    const float frequency = getParameterValue("EQ_FREQ", 1200.0f);
    const float gainDb = getParameterValue("EQ_GAIN", 0.0f);
    const float q = juce::jmax(0.1f, getParameterValue("EQ_Q", 1.0f));
    const bool enabled = getParameterValue("EQ_ON", 1.0f) >= 0.5f;

    const float centreX = frequencyToX(frequency, area);
    const float centreP = (centreX - area.getX()) / area.getWidth();
    const float width = juce::jlimit(0.025f, 0.34f, 0.22f / std::sqrt(q));

    juce::Path curve;
    for (int i = 0; i < 128; ++i)
    {
        const float p = static_cast<float>(i) / 127.0f;
        const float response = gainDb * gaussian(p, centreP, width);
        const float x = area.getX() + area.getWidth() * p;
        const float y = gainToY(response, area);

        if (i == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }

    const float zeroY = gainToY(0.0f, area);
    g.setColour(juce::Colour(0x447f8ca6));
    g.drawHorizontalLine(juce::roundToInt(zeroY), area.getX(), area.getRight());

    g.setColour((enabled ? accent : juce::Colours::grey).withAlpha(0.95f));
    g.strokePath(curve, juce::PathStrokeType(1.9f));
    g.fillEllipse(centreX - 3.5f, gainToY(gainDb, area) - 3.5f, 7.0f, 7.0f);
}

void ModulePanel::drawToneEqGraph(juce::Graphics& g, juce::Rectangle<float> area)
{
    const float low = getParameterValue("TONE_LOW", 0.0f);
    const float mid = getParameterValue("TONE_MID", 0.0f);
    const float high = getParameterValue("TONE_HIGH", 0.0f);
    const float air = getParameterValue("TONE_AIR", 0.0f);
    const float outputGain = getParameterValue("TONE_GAIN", 0.0f);
    const bool enabled = getParameterValue("TONE_ON", 1.0f) >= 0.5f;

    juce::Path curve;
    for (int i = 0; i < 128; ++i)
    {
        const float p = static_cast<float>(i) / 127.0f;

        const float lowShelf = low / (1.0f + std::exp((p - 0.27f) * 18.0f));
        const float midBell = mid * gaussian(p, 0.53f, 0.17f);
        const float highShelf = high / (1.0f + std::exp(-(p - 0.69f) * 18.0f));
        const float airShelf = air / (1.0f + std::exp(-(p - 0.86f) * 28.0f));
        const float response = lowShelf + midBell + highShelf + airShelf + outputGain;

        const float x = area.getX() + area.getWidth() * p;
        const float y = gainToY(response, area, 24.0f);

        if (i == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }

    const float zeroY = gainToY(0.0f, area, 24.0f);
    g.setColour(juce::Colour(0x447f8ca6));
    g.drawHorizontalLine(juce::roundToInt(zeroY), area.getX(), area.getRight());

    g.setColour((enabled ? accent : juce::Colours::grey).withAlpha(0.95f));
    g.strokePath(curve, juce::PathStrokeType(1.9f));
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
