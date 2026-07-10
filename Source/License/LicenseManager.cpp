#include "LicenseManager.h"

LicenseManager::Status LicenseManager::getStatus() noexcept
{
    return Status::licensed;
}

bool LicenseManager::isLicensed() noexcept
{
    return getStatus() == Status::licensed;
}

juce::String LicenseManager::getStatusText()
{
    return isLicensed() ? "Licensed to Hai Pham" : "Trial";
}
