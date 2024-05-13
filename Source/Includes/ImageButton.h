#include <utility>

//
// Created by u153171 on 12/19/2023.
//

#pragma once

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// -----------------  DO NOT CHANGE THE FOLLOWING -----------------
#if defined(_WIN32) || defined(_WIN64)
inline const char* images_folder = TOSTRING(DEFAULT_IMG_DIR);
inline const char* path_separator_ = R"(\)";
#else
inline const char* images_folder = TOSTRING(DEFAULT_IMG_DIR);
inline const char* path_separator_ = "/";
#endif

using json = nlohmann::json; // Alias for convenience
using namespace juce;

class CustomJsonImageButton : public juce::ImageButton
{
public:
    explicit CustomJsonImageButton(const nlohmann::json& ButtonJson_, // Assuming you're using nlohmann::json
                         juce::Label* sharedInfoLabel_)
    {
        ButtonJson = ButtonJson_;
        sharedInfoLabel = sharedInfoLabel_;
        if (ButtonJson.contains("isToggle")) {
            bool isToggle = ButtonJson["isToggle"].get<bool>();
            setToggleable(isToggle);
            setClickingTogglesState(isToggle);
        } else {
            setToggleable(false);
            setClickingTogglesState(false);
        }

        if (!ButtonJson_.contains("image")) {
            std::cout << "Error: No image provided for the button " + ButtonJson_["label"].get<std::string>() << std::endl;
            return;
        }

        auto loadImageFromFile = [](const std::string& path) -> std::unique_ptr<Image> {
            auto img = ImageFileFormat::loadFrom(File(path));
            if (!img.isValid())
                return nullptr;
            return std::make_unique<Image>(std::move(img));
        };

        std::string imagePath = getImagePath(ButtonJson_["image"].get<std::string>());
        if (!fileExists(imagePath)) {
            std::cout << "Error: Image file does not exist: " + imagePath << std::endl;
            std::cout << "Make sure the image is in PluginCode/img folder, then re-run the cmake command" << std::endl;
            return;
        }

        auto imagePtr = loadImageFromFile(imagePath);

        std::unique_ptr<Image> imagePressedPtr;

        if (ButtonJson_.contains("image_pressed")) {
            std::string imagePressedPath = getImagePath(ButtonJson_["image_pressed"].get<std::string>());
            if (!fileExists(imagePressedPath)) {
                std::cout << "Error: Image file does not exist: " + imagePressedPath << std::endl;
                std::cout << "Make sure the image is in PluginCode/img folder, then re-run the cmake command" << std::endl;
                return;
            }

            imagePressedPtr = loadImageFromFile(imagePressedPath);

        } else {
            imagePressedPtr = std::make_unique<Image>(*imagePtr);
        }

        // setting last alpha to 0.5f to ensure button is not clickable in transparent areas
        setImages (false, true, true, *imagePtr,
                  1.0f, {}, juce::Image(), 1.0f, {},
                  *imagePressedPtr, 1.0f, {}, 0.5f);

        // Button attachment
        auto param_name = label2ParamID(ButtonJson_["label"].get<std::string>());

    }

    void mouseEnter(const juce::MouseEvent& event) override {
        ImageButton::mouseEnter(event);

        if (sharedInfoLabel == nullptr)
            return;

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << ButtonJson["label"].get<std::string>() << " | ";
        if (ButtonJson.contains("info")) {
            auto info = ButtonJson["info"].get<std::string>();
            ss << " | " << info;
        }
        sharedInfoLabel->setText(ss.str(), juce::dontSendNotification);
    }

    void mouseExit(const juce::MouseEvent& event) override {
        ImageButton::mouseExit(event);
        if (sharedInfoLabel == nullptr)
            return;

        sharedInfoLabel->setText("", juce::dontSendNotification);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        ImageButton::paintButton(g, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }

    void buttonStateChanged() override {
        ImageButton::buttonStateChanged();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomJsonImageButton)

    juce::Label *sharedInfoLabel;
    json ButtonJson;

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


class CustomImageButton : public juce::ImageButton
{
public:
    explicit CustomImageButton(
        const std::string& not_pressed_image_path,
        const std::string& pressed_image_path)
    {


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


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomImageButton)

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
