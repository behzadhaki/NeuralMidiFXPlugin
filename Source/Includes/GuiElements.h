//
// Created by Julian Lenz on 22/12/2022.
//

#pragma once


#include "GuiParameters.h"
#include "MidiDisplayWidget.h"
#include "AudioDisplayWidget.h"

using namespace std;

// Creates the layout for a single tab
class ParameterComponent : public juce::Button::Listener,
                           public juce::Component {
public:

    explicit ParameterComponent(tab_tuple tabTuple) {
        setInterceptsMouseClicks(false, true);

        // Separate the larger tuple into a separate tuple for each category of UI elements
        tabName = std::get<0>(tabTuple);
        slidersList = std::get<1>(tabTuple);
        rotariesList = std::get<2>(tabTuple);
        buttonsList = std::get<3>(tabTuple);
        hslidersList = std::get<4>(tabTuple);
        comboBoxesList = std::get<5>(tabTuple);
        midiDisplayList = std::get<6>(tabTuple);
        audioDisplayList = std::get<7>(tabTuple);

        numButtons = buttonsList.size();
    }

    void generateGuiElements(juce::Rectangle<int> paramsArea, juce::AudioProcessorValueTreeState *apvtsPointer_) {
        setSize(paramsArea.getWidth(), paramsArea.getHeight());
        //
        apvtsPointer = apvtsPointer_;

        // Hold the string IDS to connect to APVTS in audio process thread
        std::vector<std::string> sliderParamIDS;
        std::vector<std::string> rotaryParamIDS;
        std::vector<std::string> buttonParamIDS;
        std::vector<std::string> hsliderParamIDS;

        for (const auto & sliderJson: slidersList) {
            juce::Slider *newSlider = generateSlider(sliderJson);
            auto paramID = label2ParamID(sliderJson["label"].get<std::string>());
            sliderAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    *apvtsPointer, paramID, *newSlider));
            sliderArray.add(newSlider);
            sliderTopLeftCorners.emplace_back(sliderJson["topLeftCorner"].get<std::string>());
            sliderBottomRightCorners.emplace_back(sliderJson["bottomRightCorner"].get<std::string>());
            addAndMakeVisible(newSlider);
        }

        for (const auto &rotaryJson: rotariesList) {
            juce::Slider *newRotary = generateRotary(rotaryJson);
            auto paramID = label2ParamID(rotaryJson["label"].get<std::string>());
            rotaryAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    *apvtsPointer, paramID, *newRotary));
            rotaryArray.add(newRotary);
            rotaryTopLeftCorners.emplace_back(rotaryJson["topLeftCorner"].get<std::string>());
            rotaryBottomRightCorners.emplace_back(rotaryJson["bottomRightCorner"].get<std::string>());
            addAndMakeVisible(newRotary);
        }

        for (const auto &buttonJson: buttonsList) {

            juce::TextButton *textButton = generateButton(buttonJson);
            auto isToggleable = buttonJson["isToggle"].get<bool>();
            // only need a listener for buttons that are not connected to APVTS

            auto paramID = label2ParamID(buttonJson["label"].get<std::string>());

            if (!isToggleable) {
                textButton->addListener(this);
            } else {
            buttonAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                    *apvtsPointer, paramID, *textButton));
            }

            buttonArray.add(textButton);
            buttonParameterIDs.emplace_back(paramID);
            buttonTopLeftCorners.emplace_back(buttonJson["topLeftCorner"].get<std::string>());
            buttonBottomRightCorners.emplace_back(buttonJson["bottomRightCorner"].get<std::string>());
            addAndMakeVisible(textButton);
        }

        for (const auto &sliderJson: hslidersList) {
            juce::Slider *newSlider = generateSlider(sliderJson, true);
            auto paramID = label2ParamID(sliderJson["label"].get<std::string>());
            sliderAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *apvtsPointer, paramID, *newSlider));
            sliderArray.add(newSlider);
            sliderTopLeftCorners.emplace_back(sliderJson["topLeftCorner"].get<std::string>());
            sliderBottomRightCorners.emplace_back(sliderJson["bottomRightCorner"].get<std::string>());
            addAndMakeVisible(newSlider);
        }

        for (const auto &comboJson: comboBoxesList) {
            juce::ComboBox *newComboBox = generateComboBox(comboJson);
            auto paramID = label2ParamID(comboJson["label"].get<std::string>());
            comboBoxAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                *apvtsPointer, paramID, *newComboBox));
            comboBoxArray.add(newComboBox);
            comboBoxLabelArray.add(new juce::Label);
            comboBoxLabelArray.getLast()->setText(comboJson["label"].get<std::string>(), juce::dontSendNotification);
            // set text justification
            comboBoxLabelArray.getLast()->setJustificationType(juce::Justification::centred);
            // set text color to black
            comboBoxLabelArray.getLast()->setColour(juce::Label::textColourId, juce::Colours::black);
            // comboBoxOptionsArray.push_back(std::get<1>(comboJson));
            comboBoxTopLeftCorners.emplace_back(comboJson["topLeftCorner"].get<std::string>());
            comboBoxBottomRightCorners.emplace_back(comboJson["bottomRightCorner"].get<std::string>());
            addAndMakeVisible(newComboBox);
            addAndMakeVisible(comboBoxLabelArray.getLast());
        }

        for (const auto &midiDisplayJson: midiDisplayList) {
            MidiVisualizer *newMidiVisualizer = generateMidiVisualizer(midiDisplayJson);
            midiDisplayTopLeftCorners.emplace_back(midiDisplayJson["topLeftCorner"].get<std::string>()); // [0]
            midiDisplayBottomRightCorners.emplace_back(midiDisplayJson["bottomRightCorner"].get<std::string>()); // [1]
            midiDisplayArray.add(newMidiVisualizer);
            addAndMakeVisible(newMidiVisualizer);
        }

        for (const auto &audioDisplayJson: audioDisplayList) {
            AudioVisualizer *newAudioVisualizer = generateAudioVisualizer(audioDisplayJson);
            audioDisplayTopLeftCorners.emplace_back(audioDisplayJson["topLeftCorner"].get<std::string>()); // [0]
            audioDisplayBottomRightCorners.emplace_back(audioDisplayJson["bottomRightCorner"].get<std::string>()); // [1]
            audioDisplayArray.add(newAudioVisualizer);
            addAndMakeVisible(newAudioVisualizer);
        }

    }

    void resizeGuiElements(juce::Rectangle<int> area) {

        //reshape the entire component
        setSize(area.getWidth(), area.getHeight());

        width = (float)getWidth(); // Get the width of the component
        height = (float)getHeight(); // Get the height of the component
        marginWidth = 0.05f * width; // 5% margin on both sides
        marginHeight = 0.05f * height; // 5% margin on top and bottom

        gridWidth = width - 2 * marginWidth; // Width of the grid
        gridHeight = height - 2 * marginHeight; // Height of the grid
        cellWidth = gridWidth / (float)(gridSize - 1);
        cellHeight = gridHeight / (float)(gridSize - 1);

        // Sliders
        resizeSliders();

        // Rotaries
        resizeRotaries();

        // Buttons
         resizeButtons();

         // ComboBoxes
         resizeComboBoxes();

         // MidiDisplays
         resizeMidiDisplays();

         // AudioDisplays
         resizeAudioDisplays();

         repaint();
    }

    void paint(Graphics& g) override {

        if (UIObjects::Tabs::show_grid) {
            // Draw the border around the grid
            if (UIObjects::Tabs::draw_borders_for_components) {
                g.setColour(Colours::black);
                g.drawRect(marginWidth, marginHeight, gridWidth, gridHeight, 2.0f);
            }

            for (int i = 0; i < gridSize; i++) {
                for (int j = 0; j < gridSize; j++) {
                    float x = marginWidth + static_cast<float>(i) * cellWidth; // Center of the dot
                    float y = marginHeight + static_cast<float>(j) * cellHeight; // Center of the dot

                    // Draw the dot
                    g.setColour(Colours::black);
                    g.fillEllipse(x - 2.0f, y - 2.0f, 4.0f, 4.0f); // A dot of size 4

                    // Label the first row
                    if (j == 0) {
                        char labelX = char ('A' + i);
                        String label = String::charToString(labelX);
                        g.setColour(Colours::black);
                        g.drawText(label, int(x - 10.0f), int(y - 16.0f), 20.0f, 12.0f, Justification::centred, true);
                    }

                    // Label the first column
                    if (i == 0) {
                        char labelY = char('a' + j);
                        String label = String::charToString(labelY);
                        g.setColour(Colours::black);
                        g.drawText(
                            label, int(x - 24.0f), int(y - 6.0f),
                            20.0f, 12.0f, Justification::centred, true);
                    }
                }
            }

        }

        if (UIObjects::Tabs::draw_borders_for_components) {
            g.setColour(Colours::black);
            for (const auto& border : componentBorders) {
                auto [x, y, width_, height_, tl_label, br_label] = border;
                width = width_;
                height = height_;
                g.drawRect(x, y, width, height, 2.0f);
                if (UIObjects::Tabs::show_grid) {
                    // draw labels inside the component
                    g.drawText(tl_label, int(x + 2.0f), int(y + 2.0f), 20.0f, 12.0f, Justification::centred, true);
                    g.drawText(br_label, int(x + width - 22.0f), int(y + height - 14.0f), 20.0f, 12.0f, Justification::centred, true);
                }
            }
        }

    }

    void resized() override {}

    void buttonClicked(juce::Button *button) override {
        // only need to increment button push count when
        // button is not toggleable
        if (button -> isToggleable()) { return; }
        size_t count = 0;
        for (auto b_ : buttonArray) {
            if (b_ == button) {
                auto param_pntr = apvtsPointer->getRawParameterValue(buttonParameterIDs[count]);
                *param_pntr = (*param_pntr) + 1;
                break;
            }
            count++;
        }
    }

    juce::OwnedArray<juce::Slider> sliderArray;
    juce::OwnedArray<juce::Slider> rotaryArray;
    juce::OwnedArray<juce::Button> buttonArray;
    std::vector<juce::String> buttonParameterIDs;

    juce::OwnedArray<juce::ComboBox> comboBoxArray;
    juce::OwnedArray<juce::Label> comboBoxLabelArray;

    juce::OwnedArray<MidiVisualizer> midiDisplayArray;
    juce::OwnedArray<AudioVisualizer> audioDisplayArray;

private:

    // In Runtime, APVTS used only for non-toggleable buttons
    juce::AudioProcessorValueTreeState *apvtsPointer{nullptr};

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachmentArray;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> rotaryAttachmentArray;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachmentArray;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboBoxAttachmentArray;

    std::vector<std::string> sliderTopLeftCorners;
    std::vector<std::string> sliderBottomRightCorners;
    std::vector<std::string> rotaryTopLeftCorners;
    std::vector<std::string> rotaryBottomRightCorners;
    std::vector<std::string> buttonTopLeftCorners;
    std::vector<std::string> buttonBottomRightCorners;
    std::vector<std::string> comboBoxTopLeftCorners;
    std::vector<std::string> comboBoxBottomRightCorners;
    std::vector<std::string> midiDisplayTopLeftCorners;
    std::vector<std::string> midiDisplayBottomRightCorners;
    std::vector<std::string> audioDisplayTopLeftCorners;
    std::vector<std::string> audioDisplayBottomRightCorners;

    std::vector<std::tuple<float, float, float, float, string, string>> componentBorders;

    int gridSize = 26; // Number of cells in each direction
    float width {}; // Get the width of the component
    float height {}; // Get the height of the component
    float marginWidth {}; // 5% margin on both sides
    float marginHeight {}; // 5% margin on top and bottom

    float gridWidth {}; // Width of the grid
    float gridHeight {}; // Height of the grid
    float cellWidth {};
    float cellHeight {};

    // Tuple of the current tab with all objects
    tab_tuple currentTab;
    std::string tabName;

    // Meta tuples of all objects of each element type
    slider_list slidersList;
    rotary_list rotariesList;
    button_list buttonsList;
    hslider_list hslidersList;
    comboBox_list comboBoxesList;
    midiDisplay_list midiDisplayList;
    audioDisplay_list audioDisplayList;

    size_t numButtons;

    int deltaX{};
    int deltaY{};

    static juce::Slider *generateSlider(json sliderJson_, bool isHorizontal = false) {
        auto *newSlider = new juce::Slider;
        if (isHorizontal) {
            newSlider->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        } else {
            newSlider->setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        }
        newSlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow,
                                   true,
                                   newSlider->getTextBoxWidth(),
                                   int(newSlider->getHeight()*.2));

        newSlider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
        newSlider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
        newSlider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::whitesmoke);

//        std::tie(name, minValue, maxValue, initValue, topleftCorner, bottomrightCorner) = sliderJson_;
        auto name = sliderJson_["label"].get<std::string>();
        auto minValue = sliderJson_["min"].get<double>();
        auto maxValue = sliderJson_["max"].get<double>();
        auto initValue = sliderJson_["default"].get<double>();

        // Generate labels
        juce::String str = juce::String(" ") + juce::String(name);
        newSlider->setTextValueSuffix(str);

        // Set range and initialize values
        newSlider->setRange(minValue, maxValue);
        newSlider->setValue(initValue);
        newSlider->setNumDecimalPlacesToDisplay(2);

        return newSlider;
    }

    static juce::Slider *generateRotary(json rotaryJson) {
        auto *newRotary = new juce::Slider;
        newRotary->setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);

        newRotary->setTextBoxStyle(
            juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
            newRotary->getTextBoxWidth(),
            int(newRotary->getHeight()*.2));

        newRotary->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
        newRotary->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
        newRotary->setColour(juce::Slider::textBoxTextColourId, juce::Colours::whitesmoke);
        newRotary->setColour(juce::Slider::rotarySliderFillColourId , juce::Colours::lightskyblue);

        // Retrieve single rotary and its values
        // std::tie(name, minValue, maxValue, initValue, topleftCorner, bottomrightCorner) = rotaryJson;
        auto name = rotaryJson["label"].get<std::string>();
        auto minValue = rotaryJson["min"].get<double>();
        auto maxValue = rotaryJson["max"].get<double>();
        auto initValue = rotaryJson["default"].get<double>();

        // Set Name
        juce::String str = juce::String(" ") + juce::String(name);
        newRotary->setTextValueSuffix(str);

        // Set range and initialize values
        newRotary->setRange(minValue, maxValue);
        newRotary->setValue(initValue);
        newRotary->setNumDecimalPlacesToDisplay(2);

        return newRotary;
    }

    static juce::TextButton *generateButton(json buttonJson) {
        auto *newButton = new juce::TextButton;

        // Obtain button info including text
        auto name = buttonJson["label"].get<std::string>();
        auto isToggleable = buttonJson["isToggle"].get<bool>();

        juce::String buttonText = juce::String(" ") + juce::String(name);
        newButton->setButtonText(buttonText);

        // Create toggle button if second value in tuple is true
        if (isToggleable) {
            newButton->setClickingTogglesState(true);
        }

        newButton->setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::lightblue);
        newButton->setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::black);

        return newButton;
    }

    static juce::ComboBox *generateComboBox(json comboBoxJson) {
        auto *newComboBox = new juce::ComboBox;

        // Obtain comboBox info including text
        auto name = comboBoxJson["label"].get<std::string>();
        juce::String comboBoxText = juce::String(" ") + juce::String(name);
        newComboBox->setTextWhenNothingSelected(comboBoxText);

        // Obtain comboBox options
        auto comboBoxOptions = comboBoxJson["options"].get<std::vector<std::string>>();
        int c_ = 1;
        for (auto &option : comboBoxOptions) {
            newComboBox->addItem(option, c_);
            c_++;
        }

        return newComboBox;
    }

    static MidiVisualizer *generateMidiVisualizer(json midiDisplayJson) {
        auto label = midiDisplayJson["label"].get<std::string>();
        auto allowToDragOutAsMidiFile = midiDisplayJson["allowToDragOutAsMidi"].get<bool>();
        auto allowToDragInMidiFile = midiDisplayJson["allowToDragInMidi"].get<bool>();
        auto needsPlayhead = midiDisplayJson["needsPlayhead"].get<bool>();

        auto *newMidiVisualizer = new MidiVisualizer{needsPlayhead, label};

        newMidiVisualizer->enableDragInMidi(allowToDragInMidiFile);
        newMidiVisualizer->enableDragOutAsMidi(allowToDragOutAsMidiFile);

        if (midiDisplayJson.contains("PlayheadLoopDurationQuarterNotes")) {

            auto playheadLoopDurationQuarterNotes = midiDisplayJson["PlayheadLoopDurationQuarterNotes"].get<float>();

            newMidiVisualizer->enableLooping(
                0.0f,
                playheadLoopDurationQuarterNotes);
        }

        return newMidiVisualizer;
    }

    static AudioVisualizer *generateAudioVisualizer(json audioDisplayJson) {
        auto label = audioDisplayJson["label"].get<std::string>();
        auto allowToDragOutAsAudioFile = audioDisplayJson["allowToDragOutAsAudio"].get<bool>();
        auto allowToDragInAudioFile = audioDisplayJson["allowToDragInAudio"].get<bool>();
        auto needsPlayhead = audioDisplayJson["needsPlayhead"].get<bool>();

        auto *newAudioVisualizer = new AudioVisualizer{needsPlayhead, label};

        newAudioVisualizer->enableDragInAudio(allowToDragInAudioFile);
        newAudioVisualizer->enableDragOutAsAudio(allowToDragOutAsAudioFile);

        if (audioDisplayJson.contains("PlayheadLoopDurationQuarterNotes")) {

            auto playheadLoopDurationQuarterNotes = audioDisplayJson["PlayheadLoopDurationQuarterNotes"].get<float>();

            newAudioVisualizer->enableLooping(
                0.0f,
                playheadLoopDurationQuarterNotes);
        }

        return newAudioVisualizer;
    }

    void resizeSliders() {
        int slider_ix = 0;
        componentBorders.clear();
        for (auto *comp: sliderArray) {
            comp->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                  comp->getTextBoxWidth(), int(comp->getHeight()*.2));

            auto [topLeftX, topLeftY] = coordinatesFromString(sliderTopLeftCorners[slider_ix]);
            auto [bottomRightX, bottomRightY] = coordinatesFromString(sliderBottomRightCorners[slider_ix]);

            auto x = topLeftX - 2.0f;
            auto y = topLeftY - 2.0f;
            width = (bottomRightX - topLeftX) + 4.0f;
            height = (bottomRightY - topLeftY) + 4.0f;

            comp->setBounds((int)x, (int)y, (int)width, (int)height);

            if (UIObjects::Tabs::draw_borders_for_components) {
                string tl_label = sliderTopLeftCorners[slider_ix];
                string br_label = sliderBottomRightCorners[slider_ix];
                auto border = std::tuple<float, float, float, float, string, string>(
                    x, y, width, height, tl_label, br_label);
                componentBorders.emplace_back(border);
            }

            slider_ix++;
        }
    }

    void resizeRotaries() {
        int rotary_ix = 0;
        for (auto *comp: rotaryArray)
        {
            comp->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                  comp->getTextBoxWidth(),
                                  int(comp->getHeight()*.2));

            auto [topLeftX, topLeftY] =
                coordinatesFromString(rotaryTopLeftCorners[rotary_ix]);
            auto [bottomRightX, bottomRightY] =
                coordinatesFromString(rotaryBottomRightCorners[rotary_ix]);

            auto x = topLeftX - 2.0f;
            auto y = topLeftY - 2.0f;
            width = (bottomRightX - topLeftX) + 4.0f;
            height = (bottomRightY - topLeftY) + 4.0f;

            comp->setBounds((int)x, (int)y, (int)width, (int)height);

            if (UIObjects::Tabs::draw_borders_for_components) {
                string tl_label = rotaryTopLeftCorners[rotary_ix];
                string br_label = rotaryBottomRightCorners[rotary_ix];
                auto border = std::tuple<float, float, float, float, string, string>(
                    x, y, width, height, tl_label, br_label);
                componentBorders.emplace_back(border);
            }

            rotary_ix++;
        }
    }

    void resizeButtons() {
        int button_ix = 0;
        for (auto *comp: buttonArray) {
            auto [topLeftX, topLeftY] = coordinatesFromString(buttonTopLeftCorners[button_ix]);
            auto [bottomRightX, bottomRightY] = coordinatesFromString(buttonBottomRightCorners[button_ix]);

            auto x = topLeftX - 2.0f;
            auto y = topLeftY - 2.0f;
            width = (bottomRightX - topLeftX) + 4.0f;
            height = (bottomRightY - topLeftY) + 4.0f;

            comp->setBounds((int)x, (int)y, (int)width, (int)height);

            if (UIObjects::Tabs::draw_borders_for_components) {
                string tl_label = buttonTopLeftCorners[button_ix];
                string br_label = buttonBottomRightCorners[button_ix];
                auto border = std::tuple<float, float, float, float, string, string>(
                    x, y, width, height, tl_label, br_label);
                componentBorders.emplace_back(border);
            }

            button_ix++;
        }
    }

    void resizeComboBoxes() {
        int comboBox_ix = 0;
        for (auto *comp: comboBoxArray) {
            // place the label on the left of the comboBox
            auto [topLeftX, topLeftY] = coordinatesFromString(comboBoxTopLeftCorners[comboBox_ix]);
            auto [bottomRightX, bottomRightY] = coordinatesFromString(comboBoxBottomRightCorners[comboBox_ix]);

            auto x = topLeftX - 2.0f;
            auto y = topLeftY - 2.0f;
            width = (bottomRightX - topLeftX) + 4.0f;
            height = (bottomRightY - topLeftY) + 4.0f;

            // get the label for the comboBox
            auto label = comboBoxLabelArray[comboBox_ix];
            label->setBounds((int)x, (int)y, int(width/5), int(height)); // 1/4 of the width of the comboBox

            // get the comboBox
            comp->setBounds(int(x + width/5), (int)y, int(4*width/5), (int)height); // 4/5 of the width of the comboBox

            if (UIObjects::Tabs::draw_borders_for_components) {
                string tl_label = comboBoxTopLeftCorners[comboBox_ix];
                string br_label = comboBoxBottomRightCorners[comboBox_ix];
                auto border = std::tuple<float, float, float, float, string, string>(
                    x, y, width, height, tl_label, br_label);
                componentBorders.emplace_back(border);
            }

            comboBox_ix++;
        }
    }

    void resizeMidiDisplays() {
        int midiDisplay_ix = 0;
        for (auto *comp: midiDisplayArray) {
            auto [topLeftX, topLeftY] = coordinatesFromString(midiDisplayTopLeftCorners[midiDisplay_ix]);
            auto [bottomRightX, bottomRightY] = coordinatesFromString(midiDisplayBottomRightCorners[midiDisplay_ix]);

            auto x = topLeftX - 2.0f;
            auto y = topLeftY - 2.0f;
            width = (bottomRightX - topLeftX) + 4.0f;
            height = (bottomRightY - topLeftY) + 4.0f;

            comp->setBounds((int)x, (int)y, (int)width, (int)height);

            if (UIObjects::Tabs::draw_borders_for_components) {
                string tl_label = midiDisplayTopLeftCorners[midiDisplay_ix];
                string br_label = midiDisplayBottomRightCorners[midiDisplay_ix];
                auto border = std::tuple<float, float, float, float, string, string>(
                    x, y, width, height, tl_label, br_label);
                componentBorders.emplace_back(border);
            }

            midiDisplay_ix++;
        }
    }

    void resizeAudioDisplays() {
        int audioDisplay_ix = 0;
        for (auto *comp: audioDisplayArray) {
            auto [topLeftX, topLeftY] = coordinatesFromString(audioDisplayTopLeftCorners[audioDisplay_ix]);
            auto [bottomRightX, bottomRightY] = coordinatesFromString(audioDisplayBottomRightCorners[audioDisplay_ix]);

            auto x = topLeftX - 2.0f;
            auto y = topLeftY - 2.0f;
            width = (bottomRightX - topLeftX) + 4.0f;
            height = (bottomRightY - topLeftY) + 4.0f;

            comp->setBounds((int)x, (int)y, (int)width, (int)height);

            if (UIObjects::Tabs::draw_borders_for_components) {
                string tl_label = audioDisplayTopLeftCorners[audioDisplay_ix];
                string br_label = audioDisplayBottomRightCorners[audioDisplay_ix];
                auto border = std::tuple<float, float, float, float, string, string>(
                    x, y, width, height, tl_label, br_label);
                componentBorders.emplace_back(border);
            }

            audioDisplay_ix++;
        }
    }

    std::pair<float, float> coordinatesFromString(const std::string &coordinate) {
        if (coordinate.size() != 2) {
            cout << "Invalid coordinate format - select from Aa to Zz" << endl;
            cout << "coordinate: " << coordinate << endl;
            throw std::runtime_error("Invalid coordinate format - select from Aa to Zz");
        }
        gridSize = 26; // Number of cells in each direction

        char letter_x = coordinate[0];
        char letter_y = coordinate[1];

        if ((letter_x < 'A' || letter_x > 'Z') || (letter_y < 'a' || letter_y > 'z'))
            throw std::runtime_error("Invalid coordinate value");

        width = (float)getWidth(); // Get the width of the component
        height = (float)getHeight(); // Get the height of the component
        marginWidth = 0.05f * width; // 5% margin on both sides
        marginHeight = 0.05f * height; // 5% margin on top and bottom
        gridWidth = width - 2 * marginWidth; // Width of the grid
        gridHeight = height - 2 * marginHeight; // Height of the grid
        cellWidth = gridWidth / (float)(gridSize - 1);
        cellHeight = gridHeight / (float)(gridSize - 1);

        float x = marginWidth + (float)(letter_x - 'A') * cellWidth;
        float y = marginHeight + (float)(letter_y - 'a') * cellHeight;

        return {x, y};
    }

};


