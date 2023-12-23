#pragma once

#include <array>
#include "shared_plugin_helpers/shared_plugin_helpers.h"

#pragma once

class PresetTableComponent : public juce::Component,
                             public juce::TableListBoxModel,
                             private juce::Slider::Listener
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
        currentPresetSlider.addListener(this);
        currentPresetSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, label2ParamID("Preset"), currentPresetSlider);

        // Add and configure Save and Rename buttons
        saveButton.setButtonText("Save");
        saveButton.onClick = [this] { renamePreset(); savePreset(); };
        addAndMakeVisible(saveButton);


        // Add and configure the Text Editor
        presetNameEditor.setMultiLine(false);
        presetNameEditor.setReturnKeyStartsNewLine(false);
        presetNameEditor.setReadOnly(false);
        presetNameEditor.setScrollbarsShown(true);
        presetNameEditor.setCaretVisible(true);
        presetNameEditor.setPopupMenuEnabled(true);
        addAndMakeVisible(presetNameEditor);

        loadPresetNames();

    }

    void paint(juce::Graphics& ) override
    {
        // make background light grey
        presetTables[0].setColour(juce::ListBox::backgroundColourId, juce::Colours::lightgrey);

    }

    void paintRowBackground(juce::Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll(juce::Colours::skyblue);
    }

    void paintCell(juce::Graphics& g, int rowNumber, int /*columnId*/, int width, int height, bool /*rowIsSelected*/) override
    {
        g.setColour(juce::Colours::black);
        g.drawText(presetNames[rowNumber], 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }

    ~PresetTableComponent() override = default;

    void resized() override
    {
        auto gap = proportionOfHeight(0.03f);

        auto area = getLocalBounds();
        area.removeFromTop(gap);

        auto tableArea = area.removeFromTop(proportionOfHeight(0.7f)).removeFromLeft(proportionOfWidth(1));

        for (auto& table : presetTables) {
            table.setBounds(tableArea);
            tableArea = tableArea.translated(proportionOfWidth(1), 0);
        }
        area.removeFromTop(gap);
        presetNameEditor.setBounds(area.removeFromTop(proportionOfHeight(0.05f)));
        area.removeFromTop((int)gap/2.0f);
        saveButton.setBounds(area.removeFromBottom(proportionOfHeight(0.1f)));
        area.removeFromTop(gap);

    }

    // Override to handle row selection changes
    void selectedRowsChanged(int lastRowSelected) override
    {
        if (lastRowSelected >= 0 && lastRowSelected < presetNames.size())
        {
            if ((lastRowSelected != (currentPresetSlider.getValue() - 1)) || startUp) {
                currentPresetSlider.setValue(lastRowSelected);
                presetTables[0].selectRow(lastRowSelected);
                startUp = false;
            }



            presetNameEditor.setText(presetNames[lastRowSelected], juce::dontSendNotification);
            currentPresetSlider.setValue(lastRowSelected + 1);
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
        // Step 1: Get the ValueTree from APVTS
        auto state = apvts.copyState();

        // Step 2: Serialize the ValueTree to XML
        std::unique_ptr<juce::XmlElement> xml(state.createXml());

        // Step 3: Write the XML to the file
        auto preset_idx = (int) currentPresetSlider.getValue();
        std::string fp = stripQuotes(default_preset_dir) + path_separator + std::to_string(preset_idx) + ".apvts";
        std::string fp_data = std::to_string(preset_idx) + ".preset_data";

        // Step 4: Add the file path as an attribute
        xml->setAttribute("filePath", fp_data); // Add the file path as an attribute

        // save to files
        save_tensor_map(CustomPresetData->tensors(), fp_data);
        xml->writeTo(juce::File(fp), {}); // Write the XML to the file
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
    }

    // listen to slider changes
    void sliderValueChanged(juce::Slider* slider) override {
        if (slider == &currentPresetSlider) {
            auto selectedRow = (int) currentPresetSlider.getValue() -1 ;
            if (true) {
                presetNameEditor.setText(presetNames[selectedRow], juce::dontSendNotification);
                for (auto& table : presetTables) {
                    selectedRowsChanged(selectedRow);
                    table.selectRow(selectedRow);
                }
            }
        }
    }

private:
    // used for saving presets only - Initialized in deployment thread
    CustomPresetDataDictionary *CustomPresetData;

    std::array<juce::TableListBox, 1> presetTables;
    juce::StringArray presetNames;
    juce::TextButton saveButton;
    juce::TextEditor presetNameEditor; // Added text editor for preset names
    juce::AudioProcessorValueTreeState& apvts;
    juce::Slider currentPresetSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> currentPresetSliderAttachment;

    bool startUp = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetTableComponent)
};
