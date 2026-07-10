#include "HeaderBar.h"

HeaderBar::HeaderBar()
{
    for (auto* button : { &saveButton, &deleteButton, &aButton, &bButton,
                          &compareButton, &settingsButton, &oversamplingButton, &powerButton })
    {
        configureButton(*button);
        addAndMakeVisible(*button);
    }

    saveButton.onClick = [this] { if (callbacks.savePreset) callbacks.savePreset(); };
    deleteButton.onClick = [this] { if (callbacks.deletePreset) callbacks.deletePreset(); };
    aButton.onClick = [this] { if (callbacks.selectA) callbacks.selectA(); };
    bButton.onClick = [this] { if (callbacks.selectB) callbacks.selectB(); };
    compareButton.onClick = [this] { if (callbacks.toggleAB) callbacks.toggleAB(); };
    settingsButton.onClick = [this] { if (callbacks.openSettings) callbacks.openSettings(&settingsButton); };
    oversamplingButton.onClick = [this] { if (callbacks.toggleOversampling) callbacks.toggleOversampling(); };
    powerButton.onClick = [this] { if (callbacks.togglePower) callbacks.togglePower(); };

    saveButton.setTooltip("Save current settings as User Preset");
    deleteButton.setTooltip("Delete the saved User Preset");
    aButton.setTooltip("Recall snapshot A");
    bButton.setTooltip("Recall snapshot B");
    compareButton.setTooltip("Toggle between snapshots A and B");
    settingsButton.setTooltip("Undo, Redo and interface options");
    oversamplingButton.setTooltip("Toggle 4x oversampling");
    powerButton.setTooltip("Bypass the complete vocal chain");

    refreshButtonColours();
}

void HeaderBar::setCallbacks(Callbacks newCallbacks)
{
    callbacks = std::move(newCallbacks);
}

void HeaderBar::setABState(bool useA)
{
    aIsActive = useA;
    refreshButtonColours();
}

void HeaderBar::setOversamplingActive(bool active)
{
    oversamplingIsActive = active;
    refreshButtonColours();
}

void HeaderBar::setPowerActive(bool active)
{
    powerIsActive = active;
    refreshButtonColours();
}

void HeaderBar::configureButton(juce::TextButton& button)
{
    button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff101522));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff102449));
    button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff96a1b6));
    button.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffdbe9ff));
}

void HeaderBar::refreshButtonColours()
{
    aButton.setToggleState(aIsActive, juce::dontSendNotification);
    bButton.setToggleState(!aIsActive, juce::dontSendNotification);
    oversamplingButton.setToggleState(oversamplingIsActive, juce::dontSendNotification);
    powerButton.setToggleState(powerIsActive, juce::dontSendNotification);

    repaint();
}

void HeaderBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(14, 9);
    const auto panel = bounds.toFloat();

    g.setColour(juce::Colour(0xee0b0f17));
    g.fillRoundedRectangle(panel, 14.0f);

    g.setColour(juce::Colour(0xff2b3548));
    g.drawRoundedRectangle(panel, 14.0f, 1.0f);

    auto brandArea = bounds.removeFromLeft(650).reduced(10, 0);
    auto badgeArea = brandArea.removeFromLeft(62);
    const auto badge = badgeArea.withSizeKeepingCentre(48, 48).toFloat();

    g.setColour(juce::Colour(0xff101827));
    g.fillEllipse(badge);
    g.setColour(juce::Colour(0xff34d6ff));
    g.drawEllipse(badge, 1.6f);

    juce::Path logoPath;
    logoPath.startNewSubPath(badge.getCentreX() - 8.0f, badge.getCentreY() + 12.0f);
    logoPath.lineTo(badge.getCentreX() + 12.0f, badge.getCentreY() - 9.0f);
    logoPath.lineTo(badge.getCentreX() - 10.0f, badge.getCentreY() - 14.0f);
    logoPath.closeSubPath();

    g.setColour(juce::Colour(0xffdce8ff));
    g.strokePath(logoPath, juce::PathStrokeType(2.2f));

    auto textArea = brandArea.reduced(6, 0);
    auto titleArea = textArea.removeFromTop(34);

    g.setColour(juce::Colour(0xfff2f6ff));
    g.setFont(juce::FontOptions(27.0f, juce::Font::bold));
    g.drawText("VOCAL CHAIN MASTER", titleArea, juce::Justification::centredLeft, true);

    g.setColour(juce::Colour(0xff8d99ae));
    g.setFont(juce::FontOptions(13.5f));
    g.drawText("Professional Edition", textArea, juce::Justification::centredLeft, true);
}

void HeaderBar::resized()
{
    auto right = getLocalBounds().removeFromRight(500).reduced(14, 15);

    auto place = [&right] (juce::TextButton& button, int width)
    {
        button.setBounds(right.removeFromLeft(width).reduced(4, 0));
    };

    place(saveButton, 58);
    place(deleteButton, 58);
    place(aButton, 50);
    place(bButton, 50);
    place(compareButton, 60);
    place(settingsButton, 60);
    place(oversamplingButton, 68);
    place(powerButton, 62);
}
