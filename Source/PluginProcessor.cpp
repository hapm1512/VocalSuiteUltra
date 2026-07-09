#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalSuiteUltraProAudioProcessor::VocalSuiteUltraProAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", Parameters::createLayout())
{
}

void VocalSuiteUltraProAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    dspEngine.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
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

    dspEngine.process(buffer, apvts);
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

int VocalSuiteUltraProAudioProcessor::getNumPrograms() { return 1; }
int VocalSuiteUltraProAudioProcessor::getCurrentProgram() { return 0; }
void VocalSuiteUltraProAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String VocalSuiteUltraProAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}
void VocalSuiteUltraProAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
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

