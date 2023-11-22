//
// Created by Julian Lenz on 22/12/2022.
//

#pragma once


#include "GuiParameters.h"


using namespace std;

// Creates the layout for a single tab
class ParameterComponent : public juce::Button::Listener,
                           public juce::Component {
public:

    explicit ParameterComponent(tab_tuple tabTuple) {
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

        width = getWidth(); // Get the width of the component
        height = getHeight(); // Get the height of the component
        marginWidth = 0.05f * width; // 5% margin on both sides
        marginHeight = 0.05f * height; // 5% margin on top and bottom

        gridWidth = width - 2 * marginWidth; // Width of the grid
        gridHeight = height - 2 * marginHeight; // Height of the grid
        cellWidth = gridWidth / (gridSize - 1);
        cellHeight = gridHeight / (gridSize - 1);

        // Sliders
        resizeSliders();

        // Rotaries
        resizeRotaries();

        // Buttons
         resizeButtons();

         repaint();
    }

    void paint(Graphics& g) {

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
                        char labelX = 'A' + i;
                        String label = String::charToString(labelX);
                        g.setColour(Colours::black);
                        g.drawText(label, x - 10.0f, y - 16.0f, 20.0f, 12.0f, Justification::centred, true);
                    }

                    // Label the first column
                    if (i == 0) {
                        char labelY = 'a' + j;
                        String label = String::charToString(labelY);
                        g.setColour(Colours::black);
                        g.drawText(label, x - 24.0f, y - 6.0f, 20.0f, 12.0f, Justification::centred, true);
                    }
                }
            }

        }

        if (UIObjects::Tabs::draw_borders_for_components) {
            g.setColour(Colours::black);
            for (auto border : componentBorders) {
                auto [x, y, width, height, tl_label, br_label] = border;
                g.drawRect(x, y, width, height, 2.0f);
                if (UIObjects::Tabs::show_grid) {
                    // draw labels inside the component
                    g.drawText(tl_label, x + 2.0f, y + 2.0f, 20.0f, 12.0f, Justification::centred, true);
                    g.drawText(br_label, x + width - 22.0f, y + height - 14.0f, 20.0f, 12.0f, Justification::centred, true);
                }
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

    std::string name{};
    double minValue{}, maxValue{}, initValue{};
    std::string topleftCorner{}, bottomrightCorner{};

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
    std::vector<std::tuple<float, float, float, float, string, string>> componentBorders;

    int gridSize = 26; // Number of cells in each direction
    float width; // Get the width of the component
    float height; // Get the height of the component
    float marginWidth; // 5% margin on both sides
    float marginHeight; // 5% margin on top and bottom

    float gridWidth; // Width of the grid
    float gridHeight; // Height of the grid
    float cellWidth;
    float cellHeight;

    // Tuple of the current tab with all objects
    tab_tuple currentTab;
    std::string tabName;

    // Meta tuples of all objects of each element type
    slider_list slidersList;
    rotary_list rotariesList;
    button_list buttonsList;

    size_t numSliders;
    size_t numRotaries;
    size_t numButtons;

    juce::Rectangle<int> areaWithBleed;
    int deltaX{};
    int deltaY{};

    juce::Slider *generateSlider(slider_tuple sliderTuple) {
        auto *newSlider = new juce::Slider;
        newSlider->setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        newSlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                   newSlider->getTextBoxWidth(), newSlider->getHeight()*.2);

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


    juce::Slider *generateRotary(rotary_tuple rotaryTuple) {
        auto *newRotary = new juce::Slider;
        newRotary->setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);

        newRotary->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                   newRotary->getTextBoxWidth(), newRotary->getHeight()*.2);

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

    static juce::TextButton *generateButton(button_tuple buttonTuple) {
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
        componentBorders.clear();
        for (auto *comp: sliderArray) {
            comp->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true,
                                  comp->getTextBoxWidth(), comp->getHeight()*.2);

            auto [topLeftX, topLeftY] = coordinatesFromString(sliderTopLeftCorners[slider_ix]);
            auto [bottomRightX, bottomRightY] = coordinatesFromString(sliderBottomRightCorners[slider_ix]);

            auto area = getLocalBounds();


            auto x = topLeftX - 2.0f;
            auto y = topLeftY - 2.0f;
            auto width = (bottomRightX - topLeftX) + 4.0f;
            auto height = (bottomRightY - topLeftY) + 4.0f;

            cout << "Slider " << slider_ix << " : " << x << ", " << y << ", " << width << ", " << height << endl;

            comp->setBounds(x, y, width, height);

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
                                  comp->getTextBoxWidth(), comp->getHeight()*.2);

            auto [topLeftX, topLeftY] =
                coordinatesFromString(rotaryTopLeftCorners[rotary_ix]);
            auto [bottomRightX, bottomRightY] =
                coordinatesFromString(rotaryBottomRightCorners[rotary_ix]);

            auto area = getLocalBounds();

            auto x = topLeftX - 2.0f;
            auto y = topLeftY - 2.0f;
            auto width = (bottomRightX - topLeftX) + 4.0f;
            auto height = (bottomRightY - topLeftY) + 4.0f;

            cout << "Rotary " << rotary_ix << " : " << x << ", " << y << ", " << width
                 << ", " << height << endl;

            comp->setBounds(x, y, width, height);

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

            auto area = getLocalBounds();

            auto x = topLeftX - 2.0f;
            auto y = topLeftY - 2.0f;
            auto width = (bottomRightX - topLeftX) + 4.0f;
            auto height = (bottomRightY - topLeftY) + 4.0f;

            cout << "Button " << button_ix << " : " << x << ", " << y << ", " << width << ", " << height << endl;

            comp->setBounds(x, y, width, height);

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

    std::pair<float, float> coordinatesFromString(const std::string &coordinate) {
        if (coordinate.size() != 2) {
            cout << "Invalid coordinate format - select from Aa to Zz" << endl;
            cout << "coordinate: " << coordinate << endl;
            throw std::runtime_error("Invalid coordinate format - select from Aa to Zz");
        }
        const int gridSize = 26; // Number of cells in each direction

        char letter_x = coordinate[0];
        char letter_y = coordinate[1];

        if ((letter_x < 'A' || letter_x > 'Z') || (letter_y < 'a' || letter_y > 'z'))
            throw std::runtime_error("Invalid coordinate value");

        const float width = getWidth(); // Get the width of the component
        const float height = getHeight(); // Get the height of the component
        const float marginWidth = 0.05f * width; // 5% margin on both sides
        const float marginHeight = 0.05f * height; // 5% margin on top and bottom
        const float gridWidth = width - 2 * marginWidth; // Width of the grid
        const float gridHeight = height - 2 * marginHeight; // Height of the grid
        const float cellWidth = gridWidth / (gridSize - 1);
        const float cellHeight = gridHeight / (gridSize - 1);

        float x = marginWidth + (letter_x - 'A') * cellWidth;
        float y = marginHeight + (letter_y - 'a') * cellHeight;

        return {x, y};
    }

};


