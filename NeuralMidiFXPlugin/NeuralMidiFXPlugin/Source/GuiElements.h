//
// Created by Julian Lenz on 22/12/2022.
//

#ifndef JUCECMAKEREPO_GUIELEMENTS_H
#define JUCECMAKEREPO_GUIELEMENTS_H

#include "Includes/CustomStructsAndLockFreeQueue.h"


using namespace std;


class ParameterComponent : public juce::Button::Listener,
                           public juce::Component

{
public:
    inline ParameterComponent(UIObjects::tab_tuple tabTuple) {
        // Separate the larger tuple into a separate tuple for each category of UI elements
        tabName = std::get<0>(tabTuple);
        sliderList = std::get<1>(tabTuple);
        rotaryList = std::get<2>(tabTuple);
        buttonList = std::get<3>(tabTuple);

        numSliders = sliderList.size();
        numRotaries = rotaryList.size();
        numButtons = buttonList.size();
    }

    void generateGuiElements(juce::Rectangle<int> paramsArea, juce::AudioProcessorValueTreeState *apvtsPointer) {
        setSize(paramsArea.getWidth(), paramsArea.getHeight());

        // Hold the string IDS to connect to APVTS in audio process thread
        std::vector<std::string> sliderParamIDS;
        std::vector<std::string> rotaryParamIDS;
        std::vector<std::string> buttonParamIDS;

        for (size_t i = 0; i < numSliders; i++) {
            // Retrieve single slider and its values
            auto sliderTuple = sliderList[i];

            juce::Slider* newSlider = generateSlider(sliderTuple);

            // APVTS is used to send Slider/Rotary info to the APVM thread
            auto paramID = label2ParamID(std::get<0>(sliderTuple)); // "Slider_" + to_string(i);
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> paramAttachment;
            sliderAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    *apvtsPointer, paramID, *newSlider));

            sliderArray.add(newSlider);
            addAndMakeVisible(newSlider);
        }

        for (size_t i=0; i < numRotaries; i++)
        {
            auto rotaryTuple = rotaryList[i];

            juce::Slider* newRotary = generateRotary(rotaryTuple);

            // APVTS is used to send Slider/Rotary info to the APVM thread
            auto paramID = label2ParamID(std::get<0>(rotaryTuple)); //"Rotary_" + to_string(i);
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> paramAttachment;
            rotaryAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPointer, paramID, *newRotary));

            rotaryArray.add(newRotary);
            //newRotary->addListener(this);
            addAndMakeVisible(newRotary);
        }

        for (size_t i=0; i < numButtons; i++)
        {
            auto buttonTuple = buttonList[i];
            juce::TextButton* textButton = generateButton(buttonTuple);

            // Array will be used to resize the components later on
            buttonArray.add(textButton);

            textButton->addListener(this);
            addAndMakeVisible(textButton);
        }
    }

    void resizeGuiElements(juce::Rectangle<int> area)
    {
        //reshape the entire component
        setSize(area.getWidth(), area.getHeight());

        int newHeight = int(area.getHeight() * 0.9f);
        int newWidth = int(area.getWidth() * 0.9f);

        areaWithBleed = area.withSizeKeepingCentre(newWidth, newHeight);

        //These values keep track of the moving X/Y positions of each component as they are placed
        // X values incremented in each resized() function, Y values are incremented after each function for new set of components
        deltaX = areaWithBleed.getX();
        deltaY = areaWithBleed.getY();

        // Sliders
        resizeSliders();

        // Rotaries
        deltaX = areaWithBleed.getX();
        deltaY += int((1.0f / 3.0f) * areaWithBleed.getHeight());
        resizeRotaries();

        // Buttons
        deltaX = areaWithBleed.getX();
        deltaY += int((1.0f / 3.0f) * areaWithBleed.getHeight());
        resizeButtons();
    }

    void paint(juce::Graphics& g) override {}
    void resized() override {}

    void buttonClicked(juce::Button* button) override {}

private:

    const char* name{};
    double minValue{}, maxValue{}, initValue{};

    juce::OwnedArray<juce::Slider> sliderArray;
    juce::OwnedArray<juce::Slider> rotaryArray;
    juce::OwnedArray<juce::Button> buttonArray;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachmentArray;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> rotaryAttachmentArray;

    // Tuple of the current tab with all objects
    UIObjects::tab_tuple currentTab;
    std::string tabName;

    // Meta tuples of all objects of each element type
    UIObjects::slider_list sliderList;
    UIObjects::rotary_list rotaryList;
    UIObjects::button_list buttonList;

    size_t numSliders;
    size_t numRotaries;
    size_t numButtons;

    juce::Rectangle<int> areaWithBleed;
    int deltaX;
    int deltaY;

    juce::Slider* generateSlider(UIObjects::slider_tuple sliderTuple)
    {
        auto* newSlider = new juce::Slider;
        newSlider->setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        newSlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                   newSlider->getTextBoxWidth(), 50);

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

    juce::Slider* generateRotary(UIObjects::rotary_tuple rotaryTuple)
    {
        auto* newRotary = new juce::Slider;
        newRotary->setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);

        newRotary->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                   newRotary->getTextBoxWidth(), 50);

        // Retrieve single rotary and its values
        std::tie(name, minValue,maxValue, initValue) = rotaryTuple;

        // Set Name
        juce::String str = juce::String(" ") + juce::String(name);
        newRotary->setTextValueSuffix(str);

        // Set range and initialize values
        newRotary->setRange(minValue, maxValue);
        newRotary->setValue(initValue);
        newRotary->setNumDecimalPlacesToDisplay(2);

        return newRotary;
    }

    juce::TextButton* generateButton(UIObjects::button_tuple buttonTuple)
    {
        auto* newButton = new juce::TextButton;

        // Obtain button info including text

        juce::String buttonText = juce::String(" ") + juce::String(std::get<0>(buttonTuple));
        newButton->setButtonText(buttonText);

        // Create toggle button if second value in tuple is true
        if (std::get<1>(buttonTuple))
        {
            newButton->setClickingTogglesState(true);
        }

        newButton->setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::yellow);

        return newButton;
    }

    void resizeSliders()
    {
        for (auto* comp: sliderArray)
        {
            // Initialize with X/Y position of total area, size of rectangle
            int sliderWidth = int((1.0f / numSliders) * areaWithBleed.getWidth());
            int sliderHeight = int(areaWithBleed.getHeight() / 3);

            juce::Rectangle<int> sliderSpacingArea;
            sliderSpacingArea.setSize(sliderWidth, sliderHeight);
            sliderSpacingArea.setPosition(deltaX, deltaY);

            comp->setBounds(sliderSpacingArea);

            // Increment J in proportion to number of sliders and width of rectangle
            deltaX += sliderWidth;
        }
    }

    void resizeRotaries()
    {
        for (auto* comp: rotaryArray)
        {
            // Initialize with X/Y position of total area, size of rectangle
            int rotaryWidth = int((1.0f / numRotaries) * areaWithBleed.getWidth());
            int rotaryHeight = int(areaWithBleed.getHeight() / 3);

            juce::Rectangle<int> rotarySpacingArea;
            rotarySpacingArea.setSize(rotaryWidth, rotaryHeight);
            rotarySpacingArea.setPosition(deltaX, deltaY);

            // create a slightly smaller rotary
            auto rotaryArea = rotarySpacingArea.withSizeKeepingCentre(int(rotarySpacingArea.getWidth() * 0.8f), int(rotarySpacingArea.getHeight() * 0.8f));

            comp->setBounds(rotaryArea);

            // Increment J in proportion to number of sliders and width of rectangle
            deltaX += rotaryWidth;
        }
    }

    void resizeButtons()
    {
        for (auto* comp: buttonArray)
        {
            int verticalSeparation = int(areaWithBleed.getHeight() * .08);

            int buttonWidth = int((1.0f / numButtons) * areaWithBleed.getWidth());
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


#endif //JUCECMAKEREPO_GUIELEMENTS_H
