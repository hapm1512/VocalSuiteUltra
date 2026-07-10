#pragma once

#include <juce_core/juce_core.h>

class LicenseManager final
{
public:
    enum class Status
    {
        licensed,
        trial
    };

    static Status getStatus() noexcept;
    static bool isLicensed() noexcept;
    static juce::String getStatusText();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LicenseManager)
};
