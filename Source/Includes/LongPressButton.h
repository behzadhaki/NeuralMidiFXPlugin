#include <utility>

//
// Created by u153171 on 12/19/2023.
//

#pragma once

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

//// -----------------  DO NOT CHANGE THE FOLLOWING -----------------
//#if defined(_WIN32) || defined(_WIN64)
//inline const char* images_folder = TOSTRING(DEFAULT_IMG_DIR);
//inline const char* path_separator_ = R"(\)";
//#else
//inline const char* images_folder = TOSTRING(DEFAULT_IMG_DIR);
//inline const char* path_separator_ = "/";
//#endif

using json = nlohmann::json; // Alias for convenience
using namespace juce;

class LongPressImageButton : public juce::ImageButton, private juce::Timer
{
public:
    explicit LongPressImageButton(
        const std::string& not_pressed_image_path,
        const std::string& pressed_image_path,
        juce::AudioProcessorValueTreeState* apvts_,
        const std::string& parameter_name_,
        int press_time_ms)
    {
        pressTimeMs = press_time_ms;

        apvts = apvts_;
        paramID = label2ParamID(parameter_name_);

        setToggleable(false);
        setClickingTogglesState(false);

        auto loadImageFromFile = [](const std::string& path) -> std::unique_ptr<Image> {
            auto img = ImageFileFormat::loadFrom(File(path));
            if (!img.isValid())
                return nullptr;
            return std::make_unique<Image>(std::move(img));
        };


        std::string imagePath = getImagePath(not_pressed_image_path);
        if (!fileExists(imagePath)) {
            std::cout << "[LongPressButton.h] Image file not found: " << imagePath << std::endl;
            return;
        }
        auto imageNotPressedPtr = loadImageFromFile(imagePath);

        std::string imagePressedPath = getImagePath(pressed_image_path);
        if (!fileExists(imagePressedPath)) {
            std::cout << "[LongPressButton.h] Image file not found: " << imagePressedPath << std::endl;
            return;
        }
        auto imagePressedPtr = loadImageFromFile(imagePressedPath);

        // setting last alpha to 0.5f to ensure button is not clickable in transparent areas
        setImages (false, true, true, *imageNotPressedPtr,
                  1.0f, {}, juce::Image(), 1.0f, {},
                  *imagePressedPtr, 1.0f, {}, 0.5f);


    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        ImageButton::paintButton(g, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }

    void buttonStateChanged() override {
        ImageButton::buttonStateChanged();
    }

    void timerCallback() override {
        if (isDown()) {
            if ((juce::Time::getCurrentTime() - downTime).inMilliseconds() >= pressTimeMs) {
                stopTimer();
                // Update the parameter here
                if (apvts != nullptr) {
                    auto* param = apvts->getParameter(paramID);
                    if (param != nullptr) {
                        float newValue = !param->getValue();
                        apvts->getParameter(paramID)->setValueNotifyingHost(newValue);
                    }
                }
            }
        }
    }

    void mouseDown(const juce::MouseEvent& event) override {
        Button::mouseDown(event);
        downTime = juce::Time::getCurrentTime();
        startTimer(10); // Check every 10 milliseconds
    }

    void mouseUp(const juce::MouseEvent& event) override {
        Button::mouseUp(event);
        stopTimer();
    }

    void mouseExit(const juce::MouseEvent& event) override {
        Button::mouseExit(event);
        stopTimer();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LongPressImageButton)

    juce::Time downTime;
    juce::AudioProcessorValueTreeState* apvts;
    juce::String paramID;
    int pressTimeMs;


    static std::string getImagePath(const std::string& image_name) {
        // Creates the path depending on the OS
        std::string img_path_ = stripQuotes(std::string(images_folder)) +
                                std::string(path_separator_) +
                                image_name;
        return img_path_;
    }

    static bool fileExists(const std::string& name) {
        std::ifstream f(name.c_str());
        return f.good();
    }
};

