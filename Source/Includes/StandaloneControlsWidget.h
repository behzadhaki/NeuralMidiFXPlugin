#pragma once


class StandaloneControlsWidget
    : public juce::Component,
                                   private juce::Label::Listener,
                                   public juce::Slider::Listener {
public:

    explicit StandaloneControlsWidget(juce::AudioProcessorValueTreeState& vts)
        : apvts(vts) {

        // Set up the labels
        addAndMakeVisible(numeratorValLabel);
        numeratorValLabel.setEditable(true);
        numeratorValLabel.addListener(this);
        // white border
        numeratorValLabel.setColour(juce::Label::outlineColourId, juce::Colours::white);

        addAndMakeVisible(denominatorValLabel);
        denominatorValLabel.setEditable(true);
        denominatorValLabel.addListener(this);
        // white border
        denominatorValLabel.setColour(juce::Label::outlineColourId, juce::Colours::white);

        addAndMakeVisible(meterSeparatorLabel);
        meterSeparatorLabel.setText("/", juce::dontSendNotification);
        meterSeparatorLabel.setJustificationType(juce::Justification::centred);
        meterSeparatorLabel.setEditable(false);

        // Attach the labels to the APVTS parameters
        numeratorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, label2ParamID("TimeSigNumeratorStandalone"), numeratorSlider);
        denominatorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, label2ParamID("TimeSigDenominatorStandalone"), denominatorSlider);

        numeratorValLabel.setText(std::to_string((int)numeratorSlider.getValue()), juce::dontSendNotification);
        denominatorValLabel.setText(std::to_string((int)denominatorSlider.getValue()), juce::dontSendNotification);

        // tempo
        addAndMakeVisible(tempoLabel);
        tempoLabel.setText("Tempo", juce::dontSendNotification);
        tempoLabel.setFont(juce::Font(12.0f, juce::Font::bold));
        tempoLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(tempoSlider);
        tempoSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        tempoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, label2ParamID("TempoStandalone"), tempoSlider);

        // Listen to the sliders
        numeratorSlider.addListener(this);
        denominatorSlider.addListener(this);

        // play/record buttons
        playButton.setButtonText("Play");
        playButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, label2ParamID("IsPlayingStandalone"), playButton);
        playButton.setClickingTogglesState(true);
        playButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::lightblue);
        playButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::black);

        addAndMakeVisible(playButton);

        recordButton.setButtonText("Rec");
        recordButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, label2ParamID("IsRecordingStandalone"), recordButton);
        recordButton.setClickingTogglesState(true);
        recordButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::red);
        recordButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::black);
        addAndMakeVisible(recordButton);
    }

    void paint(juce::Graphics& g) override {
        // Custom drawing code (if needed)
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override {
        // single row
        // half for tempo, half of the remaining for time sig (centered)
        // tempo slider
        auto area = getLocalBounds();

        auto edge = area.getHeight() / 4;

        area.removeFromBottom(edge);

        // leave out 20% on the left
        auto empty_area = area.removeFromLeft(area.getWidth() / 5);
        auto w = empty_area.getWidth() / 6;

        playButton.setBounds(empty_area.removeFromLeft(2*w));
        recordButton.setBounds(empty_area.removeFromRight(playButton.getWidth()));

        playButton.changeWidthToFitText();
        recordButton.changeWidthToFitText();

        // tempo area
        auto tempoArea = area.removeFromLeft(area.getWidth() / 2);
        tempoLabel.setBounds(tempoArea.removeFromLeft(tempoArea.getWidth() / 2));
        tempoSlider.setBounds(tempoArea);
        tempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, tempoArea.getWidth() / 4, tempoArea.getHeight());

        // time sig area
        auto timeSigArea = area;


        auto mW = timeSigArea.getWidth() / 6;
        timeSigArea.removeFromLeft(mW);
        numeratorValLabel.setBounds(timeSigArea.removeFromLeft(mW));
        meterSeparatorLabel.setBounds(timeSigArea.removeFromLeft(mW));
        denominatorValLabel.setBounds(timeSigArea.removeFromLeft(mW));
    }

    void sliderValueChanged(juce::Slider* slider) override {
        if (slider == &numeratorSlider) {
            numeratorValLabel.setText(std::to_string((int)numeratorSlider.getValue()), juce::dontSendNotification);
        }
        else if (slider == &denominatorSlider) {
            // get the closes valid value
            auto value = denominatorSlider.getValue();
            if (value < 2) {
                denominatorSlider.setValue(1);
            }
            else if (value < 4) {
                denominatorSlider.setValue(2);
            }
            else if (value < 8) {
                denominatorSlider.setValue(4);
            }
            else if (value < 16) {
                denominatorSlider.setValue(8);
            }
            else if (value < 32) {
                denominatorSlider.setValue(16);
            }
            else {
                denominatorSlider.setValue(32);
            }
            denominatorValLabel.setText(std::to_string((int)denominatorSlider.getValue()), juce::dontSendNotification);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Label numeratorValLabel;
    juce::Label denominatorValLabel;
    juce::Label meterSeparatorLabel;
    juce::Label tempoLabel;
    juce::Slider numeratorSlider;
    juce::Slider denominatorSlider;
    juce::Slider tempoSlider;
    juce::TextButton playButton;
    juce::TextButton recordButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> denominatorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tempoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> numeratorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playButtonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> recordButtonAttachment;

    void labelTextChanged(juce::Label* label) override {
        if (label == &numeratorValLabel) {
            auto value = numeratorValLabel.getText();
            if (isNumeratorValidMusicalValue(value)) {
                numeratorSlider.setValue(std::stoi(value.toStdString()));
            }
            else {
                numeratorValLabel.setText(std::to_string((int)numeratorSlider.getValue()), juce::dontSendNotification);
            }
        }
        else if (label == &denominatorValLabel) {
            auto value = denominatorValLabel.getText();
            if (isDenominatorValidMusicalValue(value)) {
                denominatorSlider.setValue(std::stoi(value.toStdString()));
            }
            else {
                denominatorValLabel.setText(std::to_string((int)denominatorSlider.getValue()), juce::dontSendNotification);
            }
        }
    }

    static bool isDenominatorValidMusicalValue(const String& value) {
        return value == "1" || value == "2" || value == "4" || value == "8" || value == "16" || value == "32";
    }

    static bool isNumeratorValidMusicalValue(const String& value) {
        return value == "1" || value == "2" || value == "3" || value == "4" || value == "5" || value == "6" ||
            value == "7" || value == "8" || value == "9" || value == "10" || value == "11" || value == "12" ||
            value == "13" || value == "14" || value == "15" || value == "16" || value == "17" || value == "18" ||
            value == "19" || value == "20" || value == "21" || value == "22" || value == "23" || value == "24" ||
            value == "25" || value == "26" || value == "27" || value == "28" || value == "29" || value == "30" ||
            value == "31" || value == "32";
    }
};

