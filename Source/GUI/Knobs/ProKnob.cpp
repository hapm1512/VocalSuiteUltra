#include "ProKnob.h"

ProKnob::ProKnob()
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 18);
    setRange(0.0, 100.0, 0.1);
    setValue(50.0);
    setDoubleClickReturnValue(true, 50.0);
}

void ProKnob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    auto knobArea = bounds.withTrimmedBottom(22.0f);

    const auto size = juce::jmin(knobArea.getWidth(), knobArea.getHeight());
    auto r = juce::Rectangle<float>(0, 0, size, size).withCentre(knobArea.getCentre());

    const auto centre = r.getCentre();
    const auto radius = r.getWidth() * 0.5f;

    const float start = juce::MathConstants<float>::pi * 1.25f;
    const float end   = juce::MathConstants<float>::pi * 2.75f;
    const float pos   = (float) valueToProportionOfLength(getValue());
    const float angle = start + pos * (end - start);

    g.setColour(juce::Colour(0x66000000));
    g.fillEllipse(r.translated(0.0f, 7.0f).expanded(4.0f));

    juce::Path bgArc;
    bgArc.addCentredArc(centre.x, centre.y, radius - 3.0f, radius - 3.0f,
                        0.0f, start, end, true);

    g.setColour(juce::Colour(0xff1a2230));
    g.strokePath(bgArc, juce::PathStrokeType(5.0f,
                 juce::PathStrokeType::curved,
                 juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius - 3.0f, radius - 3.0f,
                           0.0f, start, angle, true);

    g.setColour(ringColour);
    g.strokePath(valueArc, juce::PathStrokeType(5.0f,
                 juce::PathStrokeType::curved,
                 juce::PathStrokeType::rounded));

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff2e3544), r.getTopLeft(),
        juce::Colour(0xff05070b), r.getBottomRight(),
        false));
    g.fillEllipse(r.reduced(8.0f));

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff4d5a70), r.getTopLeft(),
        juce::Colour(0xff111722), r.getBottomRight(),
        false));
    g.fillEllipse(r.reduced(14.0f));

    g.setColour(juce::Colour(0x5520e6ff));
    g.drawEllipse(r.reduced(12.0f), 1.0f);

    auto pointerLength = radius * 0.48f;
    auto pointerWidth  = 3.2f;

    juce::Path pointer;
    pointer.addRoundedRectangle(-pointerWidth * 0.5f,
                                -pointerLength,
                                pointerWidth,
                                pointerLength,
                                2.0f);

    pointer.applyTransform(juce::AffineTransform::rotation(angle)
                           .translated(centre.x, centre.y));

    g.setColour(juce::Colour(0xffdff7ff));
    g.fillPath(pointer);

    for (int i = 0; i < 44; ++i)
    {
        const float p = (float)i / 43.0f;
        const float a = start + p * (end - start);

        const auto p1 = centre.getPointOnCircumference(radius - 1.0f, a);
        const auto p2 = centre.getPointOnCircumference(radius + 3.0f, a);

        g.setColour(i / 43.0f <= pos ? ringColour.withAlpha(0.75f)
                                      : juce::Colour(0xff293344));
        g.drawLine({ p1, p2 }, 1.0f);
    }

    g.setColour(juce::Colour(0xffdce8ff));
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText(juce::String(getValue(), 1),
               getLocalBounds().removeFromBottom(20),
               juce::Justification::centred);
}

