#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalSuiteUltraProAudioProcessor::VocalSuiteUltraProAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "PARAMETERS", Parameters::createLayout())
{
    stateA = apvts.copyState();
    stateB = apvts.copyState();
}

void VocalSuiteUltraProAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    dspEngine.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    dryBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock, false, false, true);
    bypassMix.reset(sampleRate, 0.02);
    bypassMix.setCurrentAndTargetValue(0.0f);
}

void VocalSuiteUltraProAudioProcessor::releaseResources()
{
    dspEngine.reset();
}

bool VocalSuiteUltraProAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && ! layouts.getMainInputChannelSet().isDisabled();
}

void VocalSuiteUltraProAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    if (dryBuffer.getNumChannels() < buffer.getNumChannels()
        || dryBuffer.getNumSamples() < buffer.getNumSamples())
    {
        dryBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        dryBuffer.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());

    dspEngine.process(buffer, apvts);

    const bool bypassed = apvts.getRawParameterValue("APP_BYPASS")->load() >= 0.5f;
    bypassMix.setTargetValue(bypassed ? 1.0f : 0.0f);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float dryAmount = bypassMix.getNextValue();
        const float wetAmount = 1.0f - dryAmount;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const float wet = buffer.getSample(channel, sample);
            const float dry = dryBuffer.getSample(channel, sample);
            buffer.setSample(channel, sample, wet * wetAmount + dry * dryAmount);
        }
    }
}


float VocalSuiteUltraProAudioProcessor::getInputPeak() const noexcept
{
    return dspEngine.getInputPeak();
}

float VocalSuiteUltraProAudioProcessor::getInputRms() const noexcept
{
    return dspEngine.getInputRms();
}

float VocalSuiteUltraProAudioProcessor::getOutputPeak() const noexcept
{
    return dspEngine.getOutputPeak();
}

float VocalSuiteUltraProAudioProcessor::getOutputRms() const noexcept
{
    return dspEngine.getOutputRms();
}

float VocalSuiteUltraProAudioProcessor::getGainReductionDb() const noexcept
{
    return dspEngine.getGainReductionDb();
}

float VocalSuiteUltraProAudioProcessor::getTruePeak() const noexcept
{
    return dspEngine.getTruePeak();
}

float VocalSuiteUltraProAudioProcessor::getTruePeakDb() const noexcept
{
    return dspEngine.getTruePeakDb();
}

float VocalSuiteUltraProAudioProcessor::getOutputPeakDb() const noexcept
{
    return dspEngine.getOutputPeakDb();
}

float VocalSuiteUltraProAudioProcessor::getOutputRmsDb() const noexcept
{
    return dspEngine.getOutputRmsDb();
}

float VocalSuiteUltraProAudioProcessor::getLufsMomentary() const noexcept
{
    return dspEngine.getLufsMomentary();
}

float VocalSuiteUltraProAudioProcessor::getLufsShortTerm() const noexcept
{
    return dspEngine.getLufsShortTerm();
}

float VocalSuiteUltraProAudioProcessor::getLufsIntegrated() const noexcept
{
    return dspEngine.getLufsIntegrated();
}

float VocalSuiteUltraProAudioProcessor::getStereoCorrelation() const noexcept
{
    return dspEngine.getStereoCorrelation();
}

float VocalSuiteUltraProAudioProcessor::getStereoWidth() const noexcept
{
    return dspEngine.getStereoWidth();
}

bool VocalSuiteUltraProAudioProcessor::copyAnalyzerBuffer(DspEngine::AnalyzerBuffer& destination) const noexcept
{
    return dspEngine.copyAnalyzerBuffer(destination);
}

juce::AudioProcessorEditor* VocalSuiteUltraProAudioProcessor::createEditor()
{
    return new VocalSuiteUltraProAudioProcessorEditor(*this);
}

bool VocalSuiteUltraProAudioProcessor::hasEditor() const { return true; }
const juce::String VocalSuiteUltraProAudioProcessor::getName() const { return "VocalSuiteUltraPro"; }

bool VocalSuiteUltraProAudioProcessor::acceptsMidi() const { return false; }
bool VocalSuiteUltraProAudioProcessor::producesMidi() const { return false; }
bool VocalSuiteUltraProAudioProcessor::isMidiEffect() const { return false; }
double VocalSuiteUltraProAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int VocalSuiteUltraProAudioProcessor::getNumPrograms()
{
    return PresetManager::factoryPresetCount;
}

int VocalSuiteUltraProAudioProcessor::getCurrentProgram()
{
    return currentProgramIndex;
}

void VocalSuiteUltraProAudioProcessor::setCurrentProgram(int index)
{
    loadFactoryPreset(index);
}

const juce::String VocalSuiteUltraProAudioProcessor::getProgramName(int index)
{
    return getFactoryPresetName(index);
}

void VocalSuiteUltraProAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void VocalSuiteUltraProAudioProcessor::loadFactoryPreset(int index)
{
    currentProgramIndex = juce::jlimit(0, PresetManager::factoryPresetCount - 1, index);
    PresetManager::applyFactoryPreset(apvts, currentProgramIndex);
}

juce::String VocalSuiteUltraProAudioProcessor::getFactoryPresetName(int index) const
{
    return PresetManager::getFactoryPresetName(index);
}

void VocalSuiteUltraProAudioProcessor::copyCurrentStateToA()
{
    stateA = apvts.copyState();
}

void VocalSuiteUltraProAudioProcessor::copyCurrentStateToB()
{
    stateB = apvts.copyState();
}

void VocalSuiteUltraProAudioProcessor::recallA()
{
    if (stateA.isValid())
        apvts.replaceState(stateA.createCopy());
}

void VocalSuiteUltraProAudioProcessor::recallB()
{
    if (stateB.isValid())
        apvts.replaceState(stateB.createCopy());
}

void VocalSuiteUltraProAudioProcessor::copyAtoB()
{
    if (stateA.isValid())
        stateB = stateA.createCopy();
}

void VocalSuiteUltraProAudioProcessor::copyBtoA()
{
    if (stateB.isValid())
        stateA = stateB.createCopy();
}


bool VocalSuiteUltraProAudioProcessor::saveUserPreset()
{
    return PresetManager::saveUserPreset(apvts);
}

bool VocalSuiteUltraProAudioProcessor::loadUserPreset()
{
    return PresetManager::loadUserPreset(apvts);
}

bool VocalSuiteUltraProAudioProcessor::deleteUserPreset()
{
    return PresetManager::deleteUserPreset();
}

bool VocalSuiteUltraProAudioProcessor::hasUserPreset() const
{
    return PresetManager::hasUserPreset();
}

bool VocalSuiteUltraProAudioProcessor::undo()
{
    return undoManager.undo();
}

bool VocalSuiteUltraProAudioProcessor::redo()
{
    return undoManager.redo();
}

void VocalSuiteUltraProAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VocalSuiteUltraProAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalSuiteUltraProAudioProcessor();
}

