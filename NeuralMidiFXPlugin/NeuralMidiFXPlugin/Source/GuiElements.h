//
// Created by Julian Lenz on 22/12/2022.
//

#pragma once


#include "Includes/CustomStructsAndLockFreeQueue.h"


using namespace std;


// Creates the layout for a single tab
class ParameterComponent : public juce::Button::Listener,
                           public juce::Component {
public:

    explicit ParameterComponent(UIObjects::tab_tuple tabTuple) {
        // Separate the larger tuple into a separate tuple for each category of UI elements
        tabName = std::get<0>(tabTuple);
        slidersList = std::get<1>(tabTuple);
        rotariesList = std::get<2>(tabTuple);
        buttonsList = std::get<3>(tabTuple);

        numSliders = slidersList.size();
        numRotaries = rotariesList.size();
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

        for (const auto &sliderTuple: slidersList) {
            juce::Slider *newSlider = generateSlider(sliderTuple);
            auto paramID = label2ParamID(std::get<0>(sliderTuple));
            sliderAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    *apvtsPointer, paramID, *newSlider));
            sliderArray.add(newSlider);
            addAndMakeVisible(newSlider);

        }

        for (const auto &rotaryTuple: rotariesList) {
            juce::Slider *newRotary = generateRotary(rotaryTuple);
            auto paramID = label2ParamID(std::get<0>(rotaryTuple));
            rotaryAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    *apvtsPointer, paramID, *newRotary));
            rotaryArray.add(newRotary);
            addAndMakeVisible(newRotary);
        }

        for (const auto &buttonTuple: buttonsList) {

            juce::TextButton *textButton = generateButton(buttonTuple);
            auto isToggleable = bool(std::get<1>(buttonTuple));
            // only need a listener for buttons that are not connected to APVTS

            auto paramID = label2ParamID(std::get<0>(buttonTuple));

            if (!isToggleable) {
                textButton->addListener(this);
            } else {
            buttonAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                    *apvtsPointer, paramID, *textButton));
            }

            buttonArray.add(textButton);
            buttonParameterIDs.push_back(paramID);

            addAndMakeVisible(textButton);
        }
    }

    void resizeGuiElements(juce::Rectangle<int> area) {
        //reshape the entire component
        setSize(area.getWidth(), area.getHeight());

        int newHeight = int(static_cast<float>(area.getHeight()) * 0.9f);
        int newWidth = int(static_cast<float>(area.getWidth()) * 0.9f);

        areaWithBleed = area.withSizeKeepingCentre(newWidth, newHeight);

        //These values keep track of the moving X/Y positions of each component as they are placed
        // X values incremented in each resized() function, Y values are incremented after each function
        // for new set of components
        deltaX = areaWithBleed.getX();
        deltaY = areaWithBleed.getY();

        // Sliders
        resizeSliders();

        // Rotaries
        deltaX = areaWithBleed.getX();
        deltaY += int((1.0f / 3.0f) * static_cast<float>(areaWithBleed.getHeight()));
        resizeRotaries();

        // Buttons
        deltaX = areaWithBleed.getX();
        deltaY += int((1.0f / 3.0f) * static_cast<float>(areaWithBleed.getHeight()));
        resizeButtons();
    }

    void paint(juce::Graphics &) override {}

    void resized() override {}

    void buttonClicked(juce::Button *button) override {
        // only need to increment button push count when
        // button is not toggeleable
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

private:

    const char *name{};
    double minValue{}, maxValue{}, initValue{};

    // In Runtime, APVTS used only for untoggleable buttons
    juce::AudioProcessorValueTreeState *apvtsPointer{nullptr};

    juce::OwnedArray<juce::Slider> sliderArray;
    juce::OwnedArray<juce::Slider> rotaryArray;
    juce::OwnedArray<juce::Button> buttonArray;
    std::vector<juce::String> buttonParameterIDs;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachmentArray;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> rotaryAttachmentArray;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachmentArray;

    // Tuple of the current tab with all objects
    UIObjects::tab_tuple currentTab;
    std::string tabName;

    // Meta tuples of all objects of each element type
    UIObjects::slider_list slidersList;
    UIObjects::rotary_list rotariesList;
    UIObjects::button_list buttonsList;

    size_t numSliders;
    size_t numRotaries;
    size_t numButtons;

    juce::Rectangle<int> areaWithBleed;
    int deltaX{};
    int deltaY{};

    juce::Slider *generateSlider(UIObjects::slider_tuple sliderTuple) {
        auto *newSlider = new juce::Slider;
        newSlider->setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        newSlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                   newSlider->getTextBoxWidth(), 60);

        newSlider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
        newSlider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
        newSlider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::whitesmoke);


        std::tie(name, minValue, maxValue, initValue) = sliderTuple;

        // Generate labels
        juce::String str = juce::String(" ") + juce::String(name);
        newSlider->setTextValueSuffix(str);

        // Set range and initialize values
        newSlider->setRange(minValue, maxValue);
        newSlider->setValue(initValue);
        newSlider->setNumDecimalPlacesToDisplay(2);

        return newSlider;
    }

    juce::Slider *generateRotary(UIObjects::rotary_tuple rotaryTuple) {
        auto *newRotary = new juce::Slider;
        newRotary->setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);

        newRotary->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                   newRotary->getTextBoxWidth(), 60);

        newRotary->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
        newRotary->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
        newRotary->setColour(juce::Slider::textBoxTextColourId, juce::Colours::whitesmoke);
        newRotary->setColour(juce::Slider::rotarySliderFillColourId , juce::Colours::lightskyblue);

        // Retrieve single rotary and its values
        std::tie(name, minValue, maxValue, initValue) = rotaryTuple;

        // Set Name
        juce::String str = juce::String(" ") + juce::String(name);
        newRotary->setTextValueSuffix(str);

        // Set range and initialize values
        newRotary->setRange(minValue, maxValue);
        newRotary->setValue(initValue);
        newRotary->setNumDecimalPlacesToDisplay(2);

        return newRotary;
    }

    static juce::TextButton *generateButton(UIObjects::button_tuple buttonTuple) {
        auto *newButton = new juce::TextButton;

        // Obtain button info including text

        juce::String buttonText = juce::String(" ") + juce::String(std::get<0>(buttonTuple));
        newButton->setButtonText(buttonText);

        // Create toggle button if second value in tuple is true
        if (std::get<1>(buttonTuple)) {
            newButton->setClickingTogglesState(true);
        }

        newButton->setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::lightblue);
        newButton->setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::black);

        return newButton;
    }

    void resizeSliders() {
        for (auto *comp: sliderArray) {
            // Initialize with X/Y position of total area, size of rectangle
            int sliderWidth = int((1.0f / static_cast<float>(numSliders)) *
                                  static_cast<float>(areaWithBleed.getWidth()));
            int sliderHeight = int(areaWithBleed.getHeight() / 3);

            juce::Rectangle<int> sliderSpacingArea;
            sliderSpacingArea.setSize(sliderWidth, sliderHeight);
            sliderSpacingArea.setPosition(deltaX, deltaY);

            comp->setBounds(sliderSpacingArea);

            // Increment J in proportion to number of sliders and width of rectangle
            deltaX += sliderWidth;
        }
    }

    void resizeRotaries() {
        for (auto *comp: rotaryArray) {
            // Initialize with X/Y position of total area, size of rectangle
            int rotaryWidth = int((1.0f / static_cast<float>(numRotaries)) *
                                  static_cast<float>(areaWithBleed.getWidth()));
            int rotaryHeight = int(areaWithBleed.getHeight() / 3);

            juce::Rectangle<int> rotarySpacingArea;
            rotarySpacingArea.setSize(rotaryWidth, rotaryHeight);
            rotarySpacingArea.setPosition(deltaX, deltaY);

            // create a slightly smaller rotary
            auto rotaryArea = rotarySpacingArea.withSizeKeepingCentre(
                    int(static_cast<float>(rotarySpacingArea.getWidth()) * 0.8f),
                    int(static_cast<float>(rotarySpacingArea.getHeight()) * 0.8f));

            comp->setBounds(rotaryArea);

            // Increment J in proportion to number of sliders and width of rectangle
            deltaX += rotaryWidth;
        }
    }

    void resizeButtons() {
        for (auto *comp: buttonArray) {
            int verticalSeparation = int(areaWithBleed.getHeight() * .08);

            int buttonWidth = int((1.0f / static_cast<float>(numButtons)) *
                    static_cast<float>(areaWithBleed.getWidth()));
            int buttonHeight = int(areaWithBleed.getHeight() / 3) - verticalSeparation;

            // This creates an overall space for each button
            juce::Rectangle<int> buttonSpacingArea;
            buttonSpacingArea.setSize(buttonWidth, buttonHeight);
            buttonSpacingArea.setPosition(deltaX, deltaY);

            // Resize button to a square
            auto buttonArea = buttonSpacingArea.withSizeKeepingCentre(buttonHeight, buttonHeight);

            comp->setBounds(buttonArea);

            deltaX += buttonWidth;
        }
    }

};


