#include "PresetBrowser.h"

PresetBrowser::PresetBrowser()
{
    configurePresetButton(userButton);
    userButton.onClick = [this]
    {
        if (userAvailable && callbacks.loadUserPreset)
        {
            callbacks.loadUserPreset();
            selectUserPreset();
        }
    };
    addAndMakeVisible(userButton);
    setUserPresetAvailable(false);
}

void PresetBrowser::setCallbacks(Callbacks newCallbacks)
{
    callbacks = std::move(newCallbacks);
}

void PresetBrowser::setFactoryPresetNames(const juce::StringArray& names)
{
    rebuildFactoryButtons(names);
}

void PresetBrowser::setUserPresetAvailable(bool available)
{
    userAvailable = available;
    userButton.setEnabled(available);
    userButton.setAlpha(available ? 1.0f : 0.45f);
    if (!available)
        userSelected = false;
    refreshSelection();
}

void PresetBrowser::setSelectedFactoryPreset(int index)
{
    selectedFactory = juce::jlimit(0, juce::jmax(0, factoryButtons.size() - 1), index);
    userSelected = false;
    refreshSelection();
}

void PresetBrowser::selectUserPreset()
{
    if (!userAvailable)
        return;

    userSelected = true;
    refreshSelection();
}

void PresetBrowser::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0x2234d6ff));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 1.0f);
}

void PresetBrowser::resized()
{
    auto area = getLocalBounds().reduced(4);
    const int total = factoryButtons.size() + 1;
    const int rowHeight = juce::jmax(18, area.getHeight() / juce::jmax(1, total));

    for (auto* button : factoryButtons)
        button->setBounds(area.removeFromTop(rowHeight).reduced(1));

    userButton.setBounds(area.removeFromTop(rowHeight).reduced(1));
}

void PresetBrowser::rebuildFactoryButtons(const juce::StringArray& names)
{
    factoryButtons.clear(true);

    for (int i = 0; i < names.size(); ++i)
    {
        auto* button = new juce::TextButton(names[i].toUpperCase());
        configurePresetButton(*button);
        button->onClick = [this, i]
        {
            if (callbacks.loadFactoryPreset)
                callbacks.loadFactoryPreset(i);
            setSelectedFactoryPreset(i);
        };
        factoryButtons.add(button);
        addAndMakeVisible(button);
    }

    refreshSelection();
    resized();
}

void PresetBrowser::refreshSelection()
{
    for (int i = 0; i < factoryButtons.size(); ++i)
        factoryButtons[i]->setToggleState(!userSelected && i == selectedFactory,
                                          juce::dontSendNotification);

    userButton.setToggleState(userSelected && userAvailable, juce::dontSendNotification);
}

void PresetBrowser::configurePresetButton(juce::TextButton& button)
{
    button.setClickingTogglesState(false);
    button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00000000));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff30245c));
    button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8d98ad));
    button.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffd9ccff));
}
