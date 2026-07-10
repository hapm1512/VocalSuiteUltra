#include "PluginEditor.h"

VocalSuiteUltraProAudioProcessorEditor::VocalSuiteUltraProAudioProcessorEditor(
    VocalSuiteUltraProAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p)
{
    configurePanels();
    configureWorkflow();
    addAllComponents();

    spectrumAnalyzer.setSampleProvider([this] (SpectrumAnalyzer::AudioSnapshot& destination)
    {
        return processor.copyAnalyzerBuffer(destination);
    });

    spectrumAnalyzer.setEqParameterSource(processor.apvts, [this]
    {
        const double currentSampleRate = processor.getSampleRate();
        return currentSampleRate > 0.0 ? currentSampleRate : 48000.0;
    });

    setSize(1536, 864);
    setResizeLimits(1200, 720, 2200, 1400);
    setResizable(true, true);
    startTimerHz(60);
}

void VocalSuiteUltraProAudioProcessorEditor::timerCallback()
{
    inputMeter.setLevel(processor.getInputPeak(), processor.getInputRms());
    outputMeter.setLevel(processor.getOutputPeak(), processor.getOutputRms());
    levelMeter.setLevel(processor.getTruePeak(), processor.getOutputRms());

    lufsMeter.setLoudness(processor.getLufsMomentary(),
                          processor.getLufsShortTerm(),
                          processor.getLufsIntegrated());

    correlationMeter.setCorrelation(processor.getStereoCorrelation(),
                                    processor.getStereoWidth());

    syncHeaderStates();
    footerBar.repaint();
}

void VocalSuiteUltraProAudioProcessorEditor::configurePanels()
{
    auto makeNames = [] (std::initializer_list<const char*> items)
    {
        juce::StringArray names;
        for (auto* item : items)
            names.add(item);
        return names;
    };

    auto makeIds = [] (std::initializer_list<const char*> items)
    {
        juce::StringArray ids;
        for (auto* item : items)
            ids.add(item);
        return ids;
    };

    auto& state = processor.apvts;

    correctionPanel.configure(state, "CORRECTION", makeNames({ "SPEED", "MIX" }),
                              makeIds({ "CORRECTION_SPEED", "CORRECTION_MIX" }),
                              "CORRECTION_ON", juce::Colour(0xff2d8cff), "KEY: D#  |  +3.2 CENT");

    noisePanel.configure(state, "NOISE REDUCTION", makeNames({ "REDUCE", "DETAIL" }),
                         makeIds({ "NOISE_REDUCE", "NOISE_DETAIL" }),
                         "NOISE_ON", juce::Colour(0xff54d941), "AI ADAPTIVE");

    preampPanel.configure(state, "PREAMP", makeNames({ "DRIVE", "BIAS", "COLOR", "OUTPUT" }),
                          makeIds({ "PREAMP_DRIVE", "PREAMP_BIAS", "PREAMP_COLOR", "PREAMP_OUTPUT" }),
                          "PREAMP_ON", juce::Colour(0xffffa326), "MODE: TUBE");

    gatePanel.configure(state, "GATE", makeNames({ "THRESH", "RANGE", "ATTACK", "RELEASE" }),
                        makeIds({ "GATE_THRESH", "GATE_RANGE", "GATE_ATTACK", "GATE_RELEASE" }),
                        "GATE_ON", juce::Colour(0xffc06bff));

    highPassPanel.configure(state, "HIGH PASS", makeNames({ "FREQ", "SLOPE" }),
                            makeIds({ "HPF_FREQ", "HPF_SLOPE" }),
                            "HPF_ON", juce::Colour(0xff34d6ff), "12 / 24 / 36 / 48 dB/OCT");

    surgicalEqPanel.configure(state, "SURGICAL EQ", makeNames({ "FREQ", "GAIN", "Q" }),
                              makeIds({ "EQ_FREQ", "EQ_GAIN", "EQ_Q" }),
                              "EQ_ON", juce::Colour(0xffffd744));

    deesserPanel.configure(state, "DE-ESSER", makeNames({ "THRESH", "REDUCE" }),
                           makeIds({ "DEESSER_THRESH", "DEESSER_REDUCE" }),
                           "DEESSER_ON", juce::Colour(0xffff72c8), "5.0 kHz - 8.0 kHz");

    compressorPanel.configure(state, "COMPRESSOR 1176",
                              makeNames({ "ATTACK", "RELEASE", "INPUT", "OUTPUT", "MIX" }),
                              makeIds({ "COMP_ATTACK", "COMP_RELEASE", "COMP_INPUT", "COMP_OUTPUT", "COMP_MIX" }),
                              "COMP_ON", juce::Colour(0xff62a8ff), "RATIO: 4:1");

    toneEqPanel.configure(state, "TONE EQ", makeNames({ "LOW", "MID", "HIGH", "AIR", "GAIN" }),
                          makeIds({ "TONE_LOW", "TONE_MID", "TONE_HIGH", "TONE_AIR", "TONE_GAIN" }),
                          "TONE_ON", juce::Colour(0xff80ee66));

    saturationPanel.configure(state, "SATURATION", makeNames({ "DRIVE", "MIX" }),
                              makeIds({ "SATURATION_DRIVE", "SATURATION_MIX" }),
                              "SATURATION_ON", juce::Colour(0xffffa326), "TYPE: TUBE");

    hiEndPanel.configure(state, "HI-END EXCITER", makeNames({ "FREQ", "AMOUNT" }),
                         makeIds({ "HIEND_FREQ", "HIEND_AMOUNT" }),
                         "HIEND_ON", juce::Colour(0xff34d6ff), "12.0 kHz");

    mixPanel.configure(state, "IN / OUT", makeNames({ "MIX" }),
                       makeIds({ "MIX_AMOUNT" }),
                       "MIX_ON", juce::Colour(0xff8db7ff));

    widthPanel.configure(state, "WIDTH", makeNames({ "WIDTH" }),
                         makeIds({ "WIDTH_AMOUNT" }),
                         "WIDTH_ON", juce::Colour(0xffb98cff), "MONO BASS: ON");
}

void VocalSuiteUltraProAudioProcessorEditor::configureWorkflow()
{
    HeaderBar::Callbacks headerCallbacks;

    headerCallbacks.savePreset = [this]
    {
        if (processor.saveUserPreset())
        {
            presetBrowser.setUserPresetAvailable(true);
            presetBrowser.selectUserPreset();
        }
    };

    headerCallbacks.deletePreset = [this]
    {
        if (processor.deleteUserPreset())
        {
            presetBrowser.setUserPresetAvailable(false);
            presetBrowser.setSelectedFactoryPreset(processor.getCurrentProgram());
        }
    };

    headerCallbacks.selectA = [this] { selectSnapshotA(); };
    headerCallbacks.selectB = [this] { selectSnapshotB(); };
    headerCallbacks.toggleAB = [this] { toggleSnapshots(); };
    headerCallbacks.openSettings = [this] (juce::Component* target) { showSettingsMenu(target); };
    headerCallbacks.toggleOversampling = [this] { toggleOversampling(); };
    headerCallbacks.togglePower = [this] { toggleGlobalBypass(); };
    headerBar.setCallbacks(std::move(headerCallbacks));

    juce::StringArray names;
    for (int i = 0; i < PresetManager::factoryPresetCount; ++i)
        names.add(processor.getFactoryPresetName(i));

    presetBrowser.setFactoryPresetNames(names);
    presetBrowser.setUserPresetAvailable(processor.hasUserPreset());
    presetBrowser.setSelectedFactoryPreset(processor.getCurrentProgram());

    PresetBrowser::Callbacks presetCallbacks;
    presetCallbacks.loadFactoryPreset = [this] (int index)
    {
        processor.loadFactoryPreset(index);
        presetBrowser.setSelectedFactoryPreset(index);
    };
    presetCallbacks.loadUserPreset = [this]
    {
        if (processor.loadUserPreset())
            presetBrowser.selectUserPreset();
    };
    presetBrowser.setCallbacks(std::move(presetCallbacks));

    syncHeaderStates();
}

void VocalSuiteUltraProAudioProcessorEditor::addAllComponents()
{
    addAndMakeVisible(headerBar);
    addAndMakeVisible(footerBar);

    for (auto* panel : { &inputPanel, &outputPanel, &spectrumPanel, &levelsPanel,
                         &lufsPanel, &correlationPanel, &presetPanel })
        addAndMakeVisible(panel);

    for (auto* panel : { &correctionPanel, &noisePanel, &preampPanel, &gatePanel, &highPassPanel,
                         &surgicalEqPanel, &deesserPanel, &compressorPanel, &toneEqPanel,
                         &saturationPanel, &hiEndPanel, &mixPanel, &widthPanel })
        addAndMakeVisible(panel);

    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(levelMeter);
    addAndMakeVisible(lufsMeter);
    addAndMakeVisible(correlationMeter);
    addAndMakeVisible(spectrumAnalyzer);
    addAndMakeVisible(presetBrowser);
}

void VocalSuiteUltraProAudioProcessorEditor::paint(juce::Graphics& g)
{
    drawBackground(g);
}

void VocalSuiteUltraProAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();

    headerArea = r.removeFromTop(74);
    footerArea = r.removeFromBottom(44);
    bodyArea = r.reduced(16);

    headerBar.setBounds(headerArea);
    footerBar.setBounds(footerArea);

    auto area = bodyArea;
    const int bottomHeight = juce::jlimit(120, 170, area.getHeight() / 5);
    auto bottom = area.removeFromBottom(bottomHeight);
    area.removeFromBottom(8);

    const int topHeight = area.getHeight() / 2;
    auto top = area.removeFromTop(topHeight);
    area.removeFromTop(8);
    auto mid = area;

    inputPanel.setBounds(top.removeFromLeft(100));
    inputMeter.setBounds(inputPanel.getBounds().reduced(25, 56));

    correctionPanel.setBounds(top.removeFromLeft(230).reduced(7, 0));
    noisePanel.setBounds(top.removeFromLeft(250).reduced(7, 0));
    preampPanel.setBounds(top.removeFromLeft(250).reduced(7, 0));
    gatePanel.setBounds(top.removeFromLeft(250).reduced(7, 0));
    highPassPanel.setBounds(top.removeFromLeft(250).reduced(7, 0));

    outputPanel.setBounds(top.reduced(7, 0));
    outputMeter.setBounds(outputPanel.getBounds().reduced(25, 56));

    surgicalEqPanel.setBounds(mid.removeFromLeft(300));
    deesserPanel.setBounds(mid.removeFromLeft(250).reduced(7, 0));
    compressorPanel.setBounds(mid.removeFromLeft(280).reduced(7, 0));
    toneEqPanel.setBounds(mid.removeFromLeft(250).reduced(7, 0));
    saturationPanel.setBounds(mid.removeFromLeft(210).reduced(7, 0));
    hiEndPanel.setBounds(mid.reduced(7, 0));

    mixPanel.setBounds(bottom.removeFromLeft(170));
    widthPanel.setBounds(bottom.removeFromLeft(180).reduced(7, 0));

    spectrumPanel.setBounds(bottom.removeFromLeft(390).reduced(7, 0));
    spectrumAnalyzer.setBounds(spectrumPanel.getBounds().reduced(14, 42));

    levelsPanel.setBounds(bottom.removeFromLeft(150).reduced(7, 0));
    levelMeter.setBounds(levelsPanel.getBounds().reduced(24, 42));

    lufsPanel.setBounds(bottom.removeFromLeft(150).reduced(7, 0));
    lufsMeter.setBounds(lufsPanel.getBounds().reduced(12, 42));

    correlationPanel.setBounds(bottom.removeFromLeft(170).reduced(7, 0));
    correlationMeter.setBounds(correlationPanel.getBounds().reduced(12, 42));

    presetPanel.setBounds(bottom.reduced(7, 0));
    presetBrowser.setBounds(presetPanel.getBounds().reduced(12, 42));
}

void VocalSuiteUltraProAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff05070d), r.getTopLeft(),
                                           juce::Colour(0xff151927), r.getBottomRight(), false));
    g.fillRect(r);

    g.setColour(juce::Colour(0x141f8cff));
    for (int i = 0; i < getWidth(); i += 64)
        g.drawVerticalLine(i, 0.0f, static_cast<float>(getHeight()));

    for (int y = 0; y < getHeight(); y += 64)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));

    g.setColour(juce::Colour(0x2200ccff));
    g.drawRect(getLocalBounds(), 1);
}

void VocalSuiteUltraProAudioProcessorEditor::showSettingsMenu(juce::Component* target)
{
    juce::PopupMenu menu;
    menu.addItem(1, "Undo", processor.undoManager.canUndo());
    menu.addItem(2, "Redo", processor.undoManager.canRedo());
    menu.addSeparator();
    menu.addItem(3, "CPU Safe Mode", true,
                 getBoolParameter(processor.apvts, "APP_CPU_SAFE"));
    menu.addItem(4, "Analyzer", true,
                 getBoolParameter(processor.apvts, "APP_ANALYZER_ON", true));

    const juce::Component::SafePointer<VocalSuiteUltraProAudioProcessorEditor> safeThis(this);
    const auto options = juce::PopupMenu::Options().withTargetComponent(target);

    menu.showMenuAsync(options, [safeThis] (int result)
    {
        auto* editor = safeThis.getComponent();
        if (editor == nullptr)
            return;

        if (result == 1)
            editor->processor.undo();
        else if (result == 2)
            editor->processor.redo();
        else if (result == 3)
        {
            const bool current = VocalSuiteUltraProAudioProcessorEditor::getBoolParameter(
                editor->processor.apvts, "APP_CPU_SAFE");
            VocalSuiteUltraProAudioProcessorEditor::setParameterValue(
                editor->processor.apvts, "APP_CPU_SAFE", current ? 0.0f : 1.0f);
        }
        else if (result == 4)
        {
            const bool current = VocalSuiteUltraProAudioProcessorEditor::getBoolParameter(
                editor->processor.apvts, "APP_ANALYZER_ON", true);
            VocalSuiteUltraProAudioProcessorEditor::setParameterValue(
                editor->processor.apvts, "APP_ANALYZER_ON", current ? 0.0f : 1.0f);
        }
    });
}

void VocalSuiteUltraProAudioProcessorEditor::selectSnapshotA()
{
    if (!snapshotAActive)
        processor.copyCurrentStateToB();

    processor.recallA();
    snapshotAActive = true;
    headerBar.setABState(true);
}

void VocalSuiteUltraProAudioProcessorEditor::selectSnapshotB()
{
    if (snapshotAActive)
        processor.copyCurrentStateToA();

    processor.recallB();
    snapshotAActive = false;
    headerBar.setABState(false);
}

void VocalSuiteUltraProAudioProcessorEditor::toggleSnapshots()
{
    if (snapshotAActive)
        selectSnapshotB();
    else
        selectSnapshotA();
}

void VocalSuiteUltraProAudioProcessorEditor::toggleOversampling()
{
    const int current = getChoiceParameterIndex(processor.apvts, "APP_OVERSAMPLING");
    setParameterValue(processor.apvts, "APP_OVERSAMPLING", current == 2 ? 0.0f : 2.0f);
    syncHeaderStates();
}

void VocalSuiteUltraProAudioProcessorEditor::toggleGlobalBypass()
{
    const bool bypassed = getBoolParameter(processor.apvts, "APP_BYPASS");
    setParameterValue(processor.apvts, "APP_BYPASS", bypassed ? 0.0f : 1.0f);
    syncHeaderStates();
}

void VocalSuiteUltraProAudioProcessorEditor::syncHeaderStates()
{
    headerBar.setABState(snapshotAActive);
    headerBar.setOversamplingActive(getChoiceParameterIndex(processor.apvts, "APP_OVERSAMPLING") == 2);
    headerBar.setPowerActive(!getBoolParameter(processor.apvts, "APP_BYPASS"));
}

bool VocalSuiteUltraProAudioProcessorEditor::getBoolParameter(
    juce::AudioProcessorValueTreeState& state,
    const juce::String& id,
    bool fallback)
{
    if (const auto* value = state.getRawParameterValue(id))
        return value->load() >= 0.5f;
    return fallback;
}

int VocalSuiteUltraProAudioProcessorEditor::getChoiceParameterIndex(
    juce::AudioProcessorValueTreeState& state,
    const juce::String& id,
    int fallback)
{
    if (const auto* value = state.getRawParameterValue(id))
        return juce::roundToInt(value->load());
    return fallback;
}

void VocalSuiteUltraProAudioProcessorEditor::setParameterValue(
    juce::AudioProcessorValueTreeState& state,
    const juce::String& id,
    float plainValue)
{
    if (auto* parameter = state.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
}
