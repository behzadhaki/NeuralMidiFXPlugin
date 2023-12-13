#include "PluginProcessor.h"
#include "PluginEditor.h"

inline double mapToLoopRange(double value, double loopStart, double loopEnd) {

    double loopDuration = loopEnd - loopStart;

    // Compute the offset relative to the loop start
    double offset = fmod(value - loopStart, loopDuration);

    // Handle negative values if 'value' is less than 'loopStart'
    if (offset < 0) offset += loopDuration;

    // Add the offset to the loop start to get the mapped value
    double mappedValue = loopStart + offset;

    return mappedValue;
}

static string label2ParamID_(const string &label) {
    // capitalize label
    string paramID = label;
    std::transform(paramID.begin(), paramID.end(), paramID.begin(), ::toupper);
    return paramID;
}

NeuralMidiFXPluginEditor::NeuralMidiFXPluginEditor(NeuralMidiFXPluginProcessor& NeuralMidiFXPluginProcessorPointer)
    : AudioProcessorEditor(&NeuralMidiFXPluginProcessorPointer),
    tabs (juce::TabbedButtonBar::Orientation::TabsAtTop)
{

    NeuralMidiFXPluginProcessorPointer_ = &NeuralMidiFXPluginProcessorPointer;
    presetManagerWidget = std::make_unique<PresetTableComponent>(
        NeuralMidiFXPluginProcessorPointer.apvts,
        NeuralMidiFXPluginProcessorPointer.deploymentThread->CustomPresetData.get());

    // Set window sizes
    setResizable (true, true);
    setSize (800, 600);

    // if standalone, add play, record, tempo, meter controls to the top
    if (UIObjects::StandaloneTransportPanel::enable) {
        playButton.setButtonText("Play/Stop");
        playButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            NeuralMidiFXPluginProcessorPointer.apvts, label2ParamID_("IsPlayingStandalone"), playButton);
        playButton.setClickingTogglesState(true);
        playButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::lightblue);
        playButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::black);
        addAndMakeVisible(playButton);

        recordButton.setButtonText("Record");
        recordButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            NeuralMidiFXPluginProcessorPointer.apvts, label2ParamID_("IsRecordingStandalone"), recordButton);
        recordButton.setClickingTogglesState(true);
        recordButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::red);
        recordButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::black);
        addAndMakeVisible(recordButton);

        tempoSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        tempoSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                    tempoSlider.getTextBoxWidth(), int(tempoSlider.getHeight()*.3));
        tempoSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            NeuralMidiFXPluginProcessorPointer.apvts, label2ParamID_("TempoStandalone"), tempoSlider);
        tempoSlider.setTextValueSuffix(juce::String("Tempo"));
        addAndMakeVisible(tempoSlider);

        numeratorSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        numeratorSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                        numeratorSlider.getTextBoxWidth(), int(numeratorSlider.getHeight()*.3));
        numeratorSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            NeuralMidiFXPluginProcessorPointer.apvts, label2ParamID_("TimeSigNumeratorStandalone"), numeratorSlider);
        numeratorSlider.setTextValueSuffix(juce::String("Num"));
        addAndMakeVisible(numeratorSlider);

        denominatorSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        denominatorSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                          denominatorSlider.getTextBoxWidth(), int(denominatorSlider.getHeight()*.3));
        denominatorSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            NeuralMidiFXPluginProcessorPointer.apvts, label2ParamID_("TimeSigDenominatorStandalone"), denominatorSlider);
        denominatorSlider.setTextValueSuffix(juce::String("Den"));
        addAndMakeVisible(denominatorSlider);

    }


    // Iteratively generate tabs. For each tab, a parent UI component of  will be created.
    for (int i = 0; i < numTabs; i++)
    {
        currentTab = UIObjects::Tabs::tabList[i];
        tabName = std::get<0>(currentTab);

        paramComponentPtr = new ParameterComponent(currentTab);

        paramComponentVector.push_back(paramComponentPtr);
        addAndMakeVisible(*paramComponentPtr);
        paramComponentPtr->generateGuiElements(
            getLocalBounds(), &NeuralMidiFXPluginProcessorPointer_->apvts);
        paramComponentPtr->resizeGuiElements(getLocalBounds());

        tabs.addTab(tabName, juce::Colours::lightgrey,
                    paramComponentPtr, true);
    }

    // add preset manager tab
    addAndMakeVisible(*presetManagerWidget);
//    tabs.addTab("Preset Manager", juce::Colours::lightgrey,
//                &presetManagerWidget, true);


    addAndMakeVisible(tabs);

    NMP2GUI_IncomingMessageSequence = NeuralMidiFXPluginProcessorPointer.NMP2GUI_IncomingMessageSequence.get();

    juce::MidiMessageSequence seq_;
    if (NMP2GUI_IncomingMessageSequence->getNumberOfWrites() > 0)
    {
        seq_ = NMP2GUI_IncomingMessageSequence->getLatestDataWithoutMovingFIFOHeads();
    } else {
        seq_ = juce::MidiMessageSequence();
    }

    double len = 8.0f * 960;

    if (UIObjects::MidiInVisualizer::enable) {
        inputPianoRoll = std::make_unique<InputMidiPianoRollComponent>(
            NeuralMidiFXPluginProcessorPointer.GUI2DPL_DroppedMidiFile_Que.get(),
            seq_);
        len = std::max(len, inputPianoRoll->getLength());
    }

    if (UIObjects::GeneratedContentVisualizer::enable) {
        outputPianoRoll = std::make_unique<OutputMidiPianoRollComponent>(
            NeuralMidiFXPluginProcessorPointer.DPL2GUI_GenerationMidiFile_Que.get());
        len = std::max(len, outputPianoRoll->getLength());
    }


    if (inputPianoRoll != nullptr) {
        inputPianoRoll->setLength(len);
        addAndMakeVisible(inputPianoRoll.get());
    }

    if (outputPianoRoll != nullptr) {
        outputPianoRoll->setLength(len);
        addAndMakeVisible(outputPianoRoll.get());
    }

    setInterceptsMouseClicks(false, true);

    startTimer(50);

    // iterate through all the tabs and set the tabbed component to the first tab
    for (int i = 0; i < (numTabs+1); i++)
    {
        tabs.setCurrentTabIndex(i);
        tabs.resized();
        tabs.setInterceptsMouseClicks(false, true);
    }
    tabs.setCurrentTabIndex(0);

    // provide resources for midi visualizers
    for (auto & i : paramComponentVector)
    {
        for (auto & mV : i->midiDisplayArray)
        {
            auto paramID = mV->getParamID();
            auto vis_data = (NeuralMidiFXPluginProcessorPointer_->midiVisualizersData.get());
            mV->setPianoRollData(vis_data->getVisualizerResources(paramID));
        }
    }

    // provide resources for audio visualizers
    for (auto & i : paramComponentVector)
    {
        for (auto & aV : i->audioDisplayArray)
        {
            auto paramID = aV->getParamID();
            auto vis_data = (NeuralMidiFXPluginProcessorPointer_->audioVisualizersData.get());
            aV->setAudioData(vis_data->getVisualizerResources(paramID));
        }
    }

    resized();
}

void NeuralMidiFXPluginEditor::resized()
{
    auto area = getLocalBounds();
    setBounds(area);
    int preset_manager_width = int(area.getWidth() * .17);

    int standalone_control_height;
    int proll_height;
    int gap;

    // place preset manager at the top
    area.removeFromLeft(int(area.getWidth() * .02));
    presetManagerWidget->setBounds(area.removeFromLeft(preset_manager_width));
    area.removeFromLeft(int(area.getWidth() * .02));

    // check if standalone
    if (UIObjects::StandaloneTransportPanel::enable) {
        standalone_control_height = int(area.getHeight() * .1);
        proll_height = int(area.getHeight() * .1);
        gap = int(area.getHeight() * .03);
    } else {
        standalone_control_height = 0;
        proll_height = int(area.getHeight() * .1);
        gap = int(area.getHeight() * .03);
    }

    if (UIObjects::StandaloneTransportPanel::enable) {
        auto controlArea = area.removeFromTop(standalone_control_height);
        auto cAreaGap = controlArea.getHeight() * .05;
        int width = int(controlArea.getWidth() * .2);

        // leave a gap above and below the standalone controls
        area.removeFromTop((int) cAreaGap);
        area.removeFromBottom((int) cAreaGap);

        // layout the standalone controls
        playButton.setBounds(controlArea.removeFromLeft(width));
        recordButton.setBounds(controlArea.removeFromLeft(width));
        tempoSlider.setBounds(controlArea.removeFromLeft(width));
        numeratorSlider.setBounds(controlArea.removeFromLeft(width));
        denominatorSlider.setBounds(controlArea.removeFromLeft(width));

    }



    if (outputPianoRoll != nullptr){
        outputPianoRoll->setBounds(area.removeFromBottom(proll_height));
    }

    if (inputPianoRoll != nullptr && outputPianoRoll != nullptr){
        area.removeFromBottom(gap);
    }

    if (inputPianoRoll != nullptr){
        inputPianoRoll->setBounds(area.removeFromBottom(proll_height));
    }

    tabs.setBounds(area);
    area.removeFromBottom(gap);
    for (auto & i : paramComponentVector)
    {
        i->resizeGuiElements(area);
    }

}

void NeuralMidiFXPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

}

void NeuralMidiFXPluginEditor::timerCallback()
{

    bool newContent = false;
    bool newPlayheadPos = false;
    auto fs_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getFs();

    if (fs_ != std::nullopt) {
        if ((*fs_ - fs) > 0.0001) {
            fs = *fs_;
            newContent = true;
        }
    }

    auto qpm_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getQpm();
    if (qpm_ != std::nullopt) {
        if ((*qpm_ - qpm) > 0.0001) {
            qpm = *qpm_;
            newContent = true;
        }
    }

    auto policy_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getPolicy();
    if (policy_ != std::nullopt) {
        play_policy = *policy_;
        newContent = true;

        if (policy_->getLoopDuration() > 0) {
            NeuralMidiFXPluginProcessorPointer_->playbackAnchorMutex.lock();
            LoopStart = NeuralMidiFXPluginProcessorPointer_->TimeAnchor.inQuarterNotes();
            NeuralMidiFXPluginProcessorPointer_->playbackAnchorMutex.unlock();
            LoopEnd = LoopStart + policy_->getLoopDuration();
            LoopingEnabled = true;
        } else {
            LoopingEnabled = false;
            LoopStart = 0;
            LoopEnd = 0;
        }
    }

    auto sequence_to_display_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getSequence();
    if (sequence_to_display_ != std::nullopt) {
        sequence_to_display = *sequence_to_display_;
        newContent = true;
    }

    auto playhead_pos_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getPlayheadPos();
    if (playhead_pos_ != std::nullopt){

        if (*playhead_pos_ != playhead_pos) {
            playhead_pos = *playhead_pos_;
            newPlayheadPos = true;
        }
    }

    if (NMP2GUI_IncomingMessageSequence->getNumReady() > 0) {
        incoming_sequence = NMP2GUI_IncomingMessageSequence->pop();
        if (inputPianoRoll != nullptr)
        {
            inputPianoRoll->displayMidiMessageSequence(incoming_sequence);
            newContent = true;
        }
    }

    if (newPlayheadPos)
    {
        if (inputPianoRoll != nullptr) {
            inputPianoRoll->displayMidiMessageSequence(playhead_pos);
        }

        if (outputPianoRoll != nullptr) {
            if (LoopingEnabled) {
                auto adjusted_playhead_pos = mapToLoopRange(playhead_pos, LoopStart, LoopEnd);
                outputPianoRoll->displayMidiMessageSequence(adjusted_playhead_pos);
            } else {
                outputPianoRoll->displayMidiMessageSequence(playhead_pos);
            }
        }

        // update playhead for user specified midi visualizers
        for (auto & i : paramComponentVector)
        {
            for (auto & mV : i->midiDisplayArray)
            {
                auto paramID = mV->getParamID();
                mV->updatePlayhead(playhead_pos);
            }
        }
    }

    if (newContent)
    {
        if (outputPianoRoll != nullptr) {
            if (LoopingEnabled) {
                outputPianoRoll->displayLoopedMidiMessageSequence(
                    sequence_to_display,
                    play_policy,
                    fs,
                    qpm,
                    LoopStart,
                    LoopEnd
                );
            } else {
                outputPianoRoll->displayMidiMessageSequence(
                    sequence_to_display,
                    play_policy,
                    fs,
                    qpm
                );
            }
        }
    }

    if (newContent || newPlayheadPos)
    {

        double len = 8.0f * 960;
        if (inputPianoRoll != nullptr) {
            len = std::max(inputPianoRoll->getLength(), len);
        }
        if (outputPianoRoll != nullptr) {
            len = std::max(outputPianoRoll->getLength(), len);
        }

        if (inputPianoRoll != nullptr) {
            inputPianoRoll->setLength(len);
            inputPianoRoll->repaint();
        }

        if (outputPianoRoll != nullptr) {
            if (LoopingEnabled) {
                len = std::max(outputPianoRoll->getMidiLength(), LoopEnd * 960);
            }
            outputPianoRoll->setLength(len);
            outputPianoRoll->repaint();
        }

        repaint();
    }

}

