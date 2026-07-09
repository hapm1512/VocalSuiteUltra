#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class GlassPanel final : public juce::Component
{
public:
    explicit GlassPanel(const juce::String& titleText);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setTitle(const juce::String& newTitle);

private:
    juce::String title;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlassPanel)
};
