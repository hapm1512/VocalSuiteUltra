#include "PresetManager.h"

juce::String PresetManager::getFactoryPresetName(int index)
{
    static const juce::StringArray names
    {
        "Vocal - Studio Pro",
        "Vocal - Male Clear",
        "Vocal - Female Bright",
        "Vocal - Rap / Trap",
        "Vocal - Ballad",
        "Vocal - Live Stage"
    };

    return names[juce::jlimit(0, factoryPresetCount - 1, index)];
}

void PresetManager::setFloat(juce::AudioProcessorValueTreeState& apvts,
                             const char* parameterId,
                             float value) noexcept
{
    if (auto* parameter = apvts.getParameter(parameterId))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void PresetManager::setBool(juce::AudioProcessorValueTreeState& apvts,
                            const char* parameterId,
                            bool value) noexcept
{
    if (auto* parameter = apvts.getParameter(parameterId))
        parameter->setValueNotifyingHost(value ? 1.0f : 0.0f);
}

void PresetManager::setChoice(juce::AudioProcessorValueTreeState& apvts,
                              const char* parameterId,
                              int value) noexcept
{
    if (auto* parameter = apvts.getParameter(parameterId))
        parameter->setValueNotifyingHost(
            parameter->convertTo0to1(static_cast<float>(value)));
}

void PresetManager::applyFactoryPreset(juce::AudioProcessorValueTreeState& apvts,
                                       int index)
{
    const int preset = juce::jlimit(0, factoryPresetCount - 1, index);

    setBool(apvts, "NOISE_ON", true);
    setBool(apvts, "PREAMP_ON", true);
    setBool(apvts, "COMP_ON", true);
    setBool(apvts, "DYN_EQ_ON", true);
    setBool(apvts, "DEESSER_ON", true);
    setBool(apvts, "LIMITER_ON", true);
    setBool(apvts, "APP_CPU_SAFE", false);

    switch (preset)
    {
        case 0:
            setFloat(apvts, "INPUT_GAIN", 0.0f);
            setFloat(apvts, "NOISE_REDUCE", 32.0f);
            setChoice(apvts, "PREAMP_MODE", 0);
            setFloat(apvts, "PREAMP_DRIVE", 26.0f);
            setFloat(apvts, "PREAMP_COLOR", 55.0f);
            setFloat(apvts, "COMP_THRESHOLD", -18.0f);
            setFloat(apvts, "COMP_MIX", 72.0f);
            setFloat(apvts, "TONE_HIGH", 1.5f);
            setFloat(apvts, "HIEND_AMOUNT", 24.0f);
            break;

        case 1:
            setFloat(apvts, "NOISE_REDUCE", 28.0f);
            setChoice(apvts, "PREAMP_MODE", 2);
            setFloat(apvts, "PREAMP_DRIVE", 18.0f);
            setFloat(apvts, "EQ_MUD_GAIN", -3.5f);
            setFloat(apvts, "TONE_LOW", -0.8f);
            setFloat(apvts, "TONE_HIGH", 1.2f);
            setFloat(apvts, "HIEND_AMOUNT", 18.0f);
            break;

        case 2:
            setFloat(apvts, "NOISE_REDUCE", 25.0f);
            setChoice(apvts, "PREAMP_MODE", 0);
            setFloat(apvts, "PREAMP_DRIVE", 16.0f);
            setFloat(apvts, "DEESSER_REDUCE", 8.5f);
            setFloat(apvts, "TONE_AIR", 2.3f);
            setFloat(apvts, "HIEND_AMOUNT", 32.0f);
            setFloat(apvts, "HIEND_AIR", 55.0f);
            break;

        case 3:
            setFloat(apvts, "NOISE_REDUCE", 38.0f);
            setChoice(apvts, "PREAMP_MODE", 1);
            setFloat(apvts, "PREAMP_DRIVE", 34.0f);
            setFloat(apvts, "COMP_THRESHOLD", -22.0f);
            setFloat(apvts, "COMP_MIX", 85.0f);
            setFloat(apvts, "SATURATION_DRIVE", 26.0f);
            setFloat(apvts, "LIMITER_DRIVE", 1.5f);
            break;

        case 4:
            setFloat(apvts, "NOISE_REDUCE", 22.0f);
            setChoice(apvts, "PREAMP_MODE", 3);
            setFloat(apvts, "PREAMP_DRIVE", 20.0f);
            setFloat(apvts, "COMP_RELEASE", 120.0f);
            setFloat(apvts, "COMP_MIX", 58.0f);
            setFloat(apvts, "TONE_MID", -0.8f);
            setFloat(apvts, "HIEND_AMOUNT", 20.0f);
            break;

        case 5:
            setFloat(apvts, "NOISE_REDUCE", 44.0f);
            setFloat(apvts, "GATE_THRESH", -42.0f);
            setFloat(apvts, "PREAMP_DRIVE", 14.0f);
            setFloat(apvts, "COMP_MIX", 65.0f);
            setFloat(apvts, "LIMITER_CEILING", -1.2f);
            setBool(apvts, "APP_CPU_SAFE", true);
            break;

        default:
            break;
    }
}

juce::File PresetManager::getUserPresetFile()
{
    auto directory =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("Vocal Chain Master")
            .getChildFile("Presets");

    if (!directory.exists())
        directory.createDirectory();

    return directory.getChildFile("User Current.vcmpreset");
}

bool PresetManager::saveUserPreset(
    juce::AudioProcessorValueTreeState& apvts)
{
    const auto file = getUserPresetFile();
    const auto state = apvts.copyState();
    const auto xml = state.createXml();

    return xml != nullptr && xml->writeTo(file);
}

bool PresetManager::loadUserPreset(
    juce::AudioProcessorValueTreeState& apvts)
{
    const auto file = getUserPresetFile();

    if (!file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse(file);

    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
        return false;

    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    return true;
}

bool PresetManager::deleteUserPreset()
{
    const auto file = getUserPresetFile();
    return !file.existsAsFile() || file.deleteFile();
}

bool PresetManager::hasUserPreset()
{
    return getUserPresetFile().existsAsFile();
}
