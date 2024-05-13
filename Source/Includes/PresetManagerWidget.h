#pragma once

#include <array>
#include "shared_plugin_helpers/shared_plugin_helpers.h"

#pragma once


class PresetTableComponent : public juce::Component,
                             public juce::TableListBoxModel,
                             private juce::Slider::Listener,
                             private juce::Timer
{
public:
    explicit PresetTableComponent(juce::AudioProcessorValueTreeState& apvts_,
                                  CustomPresetDataDictionary *prst) : apvts(apvts_), CustomPresetData(prst)
    {

        // Initialize the tables
        for (auto& table : presetTables) {
            initializeTable(table);
            addAndMakeVisible(table);
        }

        // attach the slider listener
        int preset_val = (int)*apvts.getRawParameterValue(label2ParamID("Preset"));
        currentPresetSlider.setValue(preset_val);
        currentPresetSlider.addListener(this);
        currentPresetSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, label2ParamID("Preset"), currentPresetSlider);


        // Add and configure Save and Rename buttons
        saveButton.onClick = [this] { renamePreset(); savePreset(); loadPresetNames(); repaint();};
        addAndMakeVisible(saveButton);

        // delete button
        deleteButton.onStateChange = [this] { deleteButton.isDown() ? deleteButtonPressed() : deleteButtonReleased();  };
        addAndMakeVisible(deleteButton);

        loadPresetNames();

        // Add and configure the Text Editor
        presetNameEditor.setMultiLine(false);
        presetNameEditor.setReturnKeyStartsNewLine(false);
        presetNameEditor.setReadOnly(false);
        presetNameEditor.setScrollbarsShown(true);
        presetNameEditor.setCaretVisible(true);
        presetNameEditor.setPopupMenuEnabled(true);
        auto current_preset = (int)currentPresetSlider.getValue() - 1;
        presetNameEditor.setText(presetNames[current_preset], juce::dontSendNotification);
        addAndMakeVisible(presetNameEditor);

    }

    void paint(juce::Graphics& ) override
    {
        // make background light grey
        presetTables[0].setColour(juce::ListBox::backgroundColourId, juce::Colours::lightgrey);

    }

    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
    {

    }

    void paintCell(juce::Graphics& g, int rowNumber, int /*columnId*/, int width, int height, bool rowIsSelected) override
    {
        // Fill the background
        if (rowIsSelected)
            g.fillAll(juce::Colours::lightblue);
        else
            g.fillAll(juce::Colours::transparentWhite);

        g.setColour(juce::Colours::black);
        g.drawText(presetNames[rowNumber], 2, 0, width - 4, height, juce::Justification::centredLeft, true);

        // Draw a border around the cell
        g.setColour(juce::Colours::grey.withAlpha(0.6f)); // Set the border color
        g.drawRect(0, 0, width, height, 1); // Draw the border
    }

    ~PresetTableComponent() override = default;

    void resized() override
    {
        auto gap = proportionOfHeight(0.03f);

        auto area = getLocalBounds();
        area.removeFromTop(gap);

        auto tableArea = area.removeFromTop(proportionOfHeight(0.8f)).removeFromLeft(proportionOfWidth(1));

        for (auto& table : presetTables) {
            table.setBounds(tableArea);
            tableArea = tableArea.translated(proportionOfWidth(1), 0);
        }
        area.removeFromTop(gap);
        presetNameEditor.setBounds(area.removeFromTop(proportionOfHeight(0.05f)));
        area.removeFromTop((int)gap/2.0f);
        // put save and delete buttons side by side
        auto buttonsArea = area.removeFromBottom(proportionOfHeight(0.1f));

        saveButton.setBounds(buttonsArea.removeFromLeft(proportionOfWidth(0.5f)));
        deleteButton.setBounds(buttonsArea);

        area.removeFromTop(gap);

    }

    void deleteButtonPressed() {
        // start timer
        startTimer(1000);
        deleteButtonHeldDown = true;
    }

    void deleteButtonReleased() {
        // stop timer
        stopTimer();
        deleteButtonHeldDown = false;
    }

    // Override to handle row selection changes
    void selectedRowsChanged(int lastRowSelected) override
    {

        if (lastRowSelected >= 0 && lastRowSelected < presetNames.size())
        {
            if ((lastRowSelected != (currentPresetSlider.getValue() - 1)) || startUp) {
                // currentPresetSlider.setValue(lastRowSelected);
                presetTables[0].selectRow(lastRowSelected);
                startUp = false;
                presetNameEditor.setText(presetNames[lastRowSelected], juce::dontSendNotification);
                currentPresetSlider.setValue(lastRowSelected + 1);
            }

        }
        repaint();

    }

    // save preset names to a file
    void savePresetNames()
    {
        juce::File presetNamesFile(
            stripQuotes(std::string(default_preset_dir)) + path_separator + "preset.names");
        presetNamesFile.deleteFile();
        presetNamesFile.create();
        presetNamesFile.appendText(presetNames.joinIntoString("\n"));
    }

    void loadPresetNames()
    {
        juce::File presetNamesFile(
            stripQuotes(std::string(default_preset_dir)) + path_separator + "preset.names");

        if (presetNamesFile.existsAsFile())
        {
            presetNames.clear();
            presetNamesFile.readLines(presetNames);
        }

        repaint();
    }

    void initializeTable(juce::TableListBox& table)
    {
        // Add a single column
        table.getHeader().addColumn("Preset", 1, 100);

        // Set up table propertie
        table.setModel(this);
        table.setColour(juce::ListBox::outlineColourId, juce::Colours::grey);
        table.setOutlineThickness(1);
        table.setMultipleSelectionEnabled(false);

        // Populate the table with presets
        for (int i = 0; i < 100; ++i) {
            presetNames.add("---");
        }

        table.setRowHeight(20);

    }

    // TableListBoxModel overrides
    int getNumRows() override
    {
        return presetNames.size();
    }

    /*void loadPreset() {
        // moved to APVTSMediatorThread
    }*/

    void savePreset()
    {
        // Generate a random 6-character string
        auto generateRandomString = []() -> std::string {
            const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            const size_t max_index = (sizeof(charset) - 1);
            std::string ret;
            for (int i = 0; i < 6; ++i) {
                ret += charset[rand() % max_index];
            }
            return ret;
        };

        // Step 1: Get the ValueTree from APVTS
        auto state = apvts.copyState();

        // Step 2: Serialize the ValueTree to XML
        std::unique_ptr<juce::XmlElement> xml(state.createXml());

        // Step 3: Prepare file paths
        auto preset_idx = (int)currentPresetSlider.getValue();
        juce::String fp = stripQuotes(default_preset_dir) + juce::File::getSeparatorString() + juce::String(preset_idx) + ".apvts";
        // preset data is the same just replace extension
        juce::String preset_data_fname = juce::String(preset_idx) + ".preset_data";
        juce::String fp_data = stripQuotes(default_preset_dir) + juce::File::getSeparatorString() + preset_data_fname;

        // Create "deleted" folder if it doesn't exist
        juce::File deletedFolder(fp);
        deletedFolder = deletedFolder.getParentDirectory().getChildFile("deleted");
        if (!deletedFolder.exists()) {
            deletedFolder.createDirectory();
        }

        // Move existing files to "deleted" folder
        juce::File apvtsFile(fp);
        juce::File presetDataFile(fp_data);
        if (apvtsFile.exists()) {
            apvtsFile.moveFileTo(deletedFolder.getChildFile(apvtsFile.getFileNameWithoutExtension() + "_" + generateRandomString() + apvtsFile.getFileExtension()));
        }
        if (presetDataFile.exists()) {
            presetDataFile.moveFileTo(deletedFolder.getChildFile(presetDataFile.getFileNameWithoutExtension() + "_" + generateRandomString() + presetDataFile.getFileExtension()));
        }

        // Step 4: Add the file path as an attribute
        xml->setAttribute("filePath", preset_data_fname); // Add the file path as an attribute

        // Save to files
        save_tensor_map(CustomPresetData->tensors(), fp_data.toStdString());
        xml->writeTo(juce::File(fp), {}); // Write the XML to the file

        // Step 5: Update the preset names
        presetNames.set(preset_idx - 1, presetNameEditor.getText());

        // Repaint
        repaint();
    }

    void renamePreset()
    {
        for (auto& table : presetTables) {

            // if text has --- in it, replace it with the "Preset " + preset number
            if (presetNameEditor.getText().contains("---")) {
                auto selectedRow = table.getSelectedRow();
                presetNames.set(selectedRow, "Preset " + juce::String(selectedRow + 1));
                table.updateContent();
                break;
            }

            if (table.getSelectedRow() >= 0) {
                auto selectedRow = table.getSelectedRow();
                presetNames.set(selectedRow, presetNameEditor.getText());
                table.updateContent();
                break;
            }
        }

        savePresetNames();
        loadPresetNames();

        repaint();
    }

    // listen to slider changes
    void sliderValueChanged(juce::Slider* slider) override {
        if (slider == &currentPresetSlider) {
            auto selectedRow = (int) currentPresetSlider.getValue() -1 ;
            presetNameEditor.setText(presetNames[selectedRow], juce::dontSendNotification);
            for (auto& table : presetTables) {
                selectedRowsChanged(selectedRow);
                table.selectRow(selectedRow);
            }
        }
    }

    void timerCallback() override {
        // todo implement delete preset if button is held down over 3 seconds
    }

private:
    // used for saving presets only - Initialized in deployment thread
    CustomPresetDataDictionary *CustomPresetData;

    std::array<juce::TableListBox, 1> presetTables;
    juce::StringArray presetNames;
    CustomImageButton saveButton{"SavePresetNotPressed.png", "SavePresetPressed.png"};
    CustomImageButton deleteButton{"DeletePresetNotPressed.png", "DeletePresetPressed.png"};
    bool deleteButtonHeldDown = false;

    juce::TextEditor presetNameEditor; // Added text editor for preset names
    juce::AudioProcessorValueTreeState& apvts;
    juce::Slider currentPresetSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> currentPresetSliderAttachment;

    bool startUp = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetTableComponent)
};
