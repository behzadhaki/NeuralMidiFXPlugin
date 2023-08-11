//
// Created by Julian Lenz on 22/12/2022.
//

#pragma once


#include "GuiParameters.h"


using namespace std;

inline std::pair<int, int> coordinatesFromString(const std::string &coordinate) {
    if (coordinate.size() != 2)
        throw std::runtime_error("Invalid coordinate format");

    char letter = coordinate[0];
    char number = coordinate[1];

    if (letter < 'A' || letter > 'Z' || number < '1' || number > '9')
        throw std::runtime_error("Invalid coordinate value");

    int x = letter - 'A'; // Convert letter to 0-based index
    int y = number - '1'; // Convert number to 0-based index

    return {x, y};
}

inline void place(juce::Component &component, const juce::Rectangle<int> &area,
           const std::string &topLeftCorner, const std::string &bottomRightCorner) {

    auto [topLeftX, topLeftY] = coordinatesFromString(topLeftCorner);
    auto [bottomRightX, bottomRightY] = coordinatesFromString(bottomRightCorner);

    int cellWidth = area.getWidth() / ('Z' - 'A' + 1);
    int cellHeight = area.getHeight() / (9 - 1 + 1);

    int x = topLeftX * cellWidth;
    int y = topLeftY * cellHeight;
    int width = (bottomRightX - topLeftX + 1) * cellWidth;
    int height = (bottomRightY - topLeftY + 1) * cellHeight;

    component.setBounds(x, y, width, height);
}

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
            sliderTopLeftCorners.emplace_back(std::get<4>(sliderTuple));
            sliderBottomRightCorners.emplace_back(std::get<5>(sliderTuple));
            addAndMakeVisible(newSlider);
        }

        for (const auto &rotaryTuple: rotariesList) {
            juce::Slider *newRotary = generateRotary(rotaryTuple);
            auto paramID = label2ParamID(std::get<0>(rotaryTuple));
            rotaryAttachmentArray.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    *apvtsPointer, paramID, *newRotary));
            rotaryArray.add(newRotary);
            rotaryTopLeftCorners.emplace_back(std::get<4>(rotaryTuple));
            rotaryBottomRightCorners.emplace_back(std::get<5>(rotaryTuple));
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
            buttonParameterIDs.emplace_back(paramID);
            buttonTopLeftCorners.emplace_back(std::get<2>(buttonTuple));
            buttonBottomRightCorners.emplace_back(std::get<3>(buttonTuple));
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
//        deltaX = areaWithBleed.getX();
//        deltaY += int((1.0f / 3.0f) * static_cast<float>(areaWithBleed.getHeight()));
        // resizeRotaries();

        // Buttons
//        deltaX = areaWithBleed.getX();
//        deltaY += int((1.0f / 3.0f) * static_cast<float>(areaWithBleed.getHeight()));
        // resizeButtons();

        repaint();
    }

    void paint(Graphics& g) {
        if (!UIObjects::Tabs::show_grid) {
            return;
        }

        const int gridSize = 26; // Number of cells in each direction
        const float width = getWidth(); // Get the width of the component
        const float height = getHeight(); // Get the height of the component
        const float cellWidth = width / gridSize;
        const float cellHeight = height / gridSize;

        for (int i = 0; i < gridSize; i++) {
            for (int j = 0; j < gridSize; j++) {
            float x = static_cast<float>(i) * cellWidth; // Top-left corner of the cell
            float y = static_cast<float>(j) * cellHeight; // Top-left corner of the cell

            // Draw the dot at the top-left corner of the cell
            g.setColour(Colours::black);
            g.fillEllipse(x, y, 4.0f, 4.0f); // A dot of size 4

            // Label the first row
            if (j == 0) {
                char labelX = 'A' + i;
                String label = String::charToString(labelX);
                g.setColour(Colours::black);
                g.drawText(label, x, y - 16.0f, 20.0f, 12.0f, Justification::centred, true);
            }

            // Label the first column
            if (i == 0) {
                char labelY = 'a' + j;
                String label = String::charToString(labelY);
                g.setColour(Colours::black);
                g.drawText(label, x - 16.0f, y, 20.0f, 12.0f, Justification::centred, true);
            }
            }
        }

        if (UIObjects::Tabs::draw_borders_for_components) {
            g.setColour(Colours::black);
            for (auto &border: componentBorders) {
            float x = std::get<0>(border);
            float y = std::get<1>(border);
            float w = std::get<2>(border);
            float h = std::get<3>(border);
            Rectangle<float> rect(x, y, w, h);
            g.drawRect(rect, 1.0f);
            }
        }
    }


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
    const char *topleftCorner{}, *bottomrightCorner{};

    // In Runtime, APVTS used only for untoggleable buttons
    juce::AudioProcessorValueTreeState *apvtsPointer{nullptr};

    juce::OwnedArray<juce::Slider> sliderArray;
    juce::OwnedArray<juce::Slider> rotaryArray;
    juce::OwnedArray<juce::Button> buttonArray;
    std::vector<juce::String> buttonParameterIDs;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachmentArray;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> rotaryAttachmentArray;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachmentArray;


    std::vector<std::string> sliderTopLeftCorners;
    std::vector<std::string> sliderBottomRightCorners;
    std::vector<std::string> rotaryTopLeftCorners;
    std::vector<std::string> rotaryBottomRightCorners;
    std::vector<std::string> buttonTopLeftCorners;
    std::vector<std::string> buttonBottomRightCorners;
    std::vector<std::tuple<int, int, int, int>> componentBorders;

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


        std::tie(name, minValue, maxValue, initValue, topleftCorner, bottomrightCorner) = sliderTuple;

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
        std::tie(name, minValue, maxValue, initValue, topleftCorner, bottomrightCorner) = rotaryTuple;

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
        int slider_ix = 0;
        for (auto *comp: sliderArray) {
//            // Initialize with X/Y position of total area, size of rectangle
//            int sliderWidth = int((1.0f / static_cast<float>(numSliders)) *
//                                  static_cast<float>(areaWithBleed.getWidth()));
//            int sliderHeight = int(areaWithBleed.getHeight() / 3);
//
//            juce::Rectangle<int> sliderSpacingArea;
//            sliderSpacingArea.setSize(sliderWidth, sliderHeight);
//            sliderSpacingArea.setPosition(deltaX, deltaY);
//
//            comp->setBounds(sliderSpacingArea);
//
//            // Increment J in proportion to number of sliders and width of rectangle
//            deltaX += sliderWidth;
            auto [topLeftX, topLeftY] = coordinatesFromString(sliderTopLeftCorners[slider_ix]);
            auto [bottomRightX, bottomRightY] = coordinatesFromString(sliderBottomRightCorners[slider_ix]);

            auto area = getLocalBounds();


            int x = topLeftX * area.getWidth();
            int y = topLeftY * area.getHeight();
            int width = (bottomRightX - topLeftX) * area.getWidth();
            int height = (bottomRightY - topLeftY) * area.getHeight();

            cout << "Slider " << slider_ix << " : " << x << ", " << y << ", " << width << ", " << height << endl;

            comp->setBounds(x, y, width, height);

            if (UIObjects::Tabs::draw_borders_for_components) {
                auto border = std::tuple<int, int, int, int>(x, y, width, height);
                componentBorders.emplace_back(border);
            }
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

    std::pair<float, float> coordinatesFromString(const std::string &coordinate) {
        if (coordinate.size() != 2)
        {
            cout << "Invalid coordinate format - select from Aa to Zz"  << endl;
            throw std::runtime_error("Invalid coordinate format - select from Aa to Zz");
        }
        char letter_x = coordinate[0];
        char letter_y = coordinate[1];

        if ((letter_x < 'A' || letter_x > 'Z') || (letter_y < 'a' || letter_y > 'z'))
            throw std::runtime_error("Invalid coordinate value");

        float x = (letter_x - 'A') / 25.0f; // Convert letter to ratio, 'A' to 'Z' -> 0 to 1
        float y = (letter_y - 'a') / 25.0f; // Convert letter to ratio, 'a' to 'z' -> 0 to 1

        return {x, y};
    }

    void place(juce::Component &component,
               const std::string &topLeftCorner, const std::string &bottomRightCorner) {

        auto area = getLocalBounds();
        auto [topLeftX, topLeftY] = coordinatesFromString(topLeftCorner);
        auto [bottomRightX, bottomRightY] = coordinatesFromString(bottomRightCorner);

        int cellWidth = area.getWidth() / ('Z' - 'A' + 1);
        int cellHeight = area.getHeight() / (9 - 1 + 1);

        int x = topLeftX * cellWidth;
        int y = topLeftY * cellHeight;
        int width = (bottomRightX - topLeftX + 1) * cellWidth;
        int height = (bottomRightY - topLeftY + 1) * cellHeight;

        component.setBounds(x, y, width, height);
    }

};


