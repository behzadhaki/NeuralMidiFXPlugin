### mVirtualMidiOut

- Creates a virtual midi port that always contains the content of the plugin midi buffer at any given time
- can now be enabled via configs_GUI.h
- Only OSX supported for this feature

### Standalone Mode

- Now can run in standalone mode
- Can be enabled via configs_GUI.h

### MAJOR UPDATE

- Settings are now saved in a file called "settings.json" rather than the settings Configs_GUI.h file
- there is now a single thread version of the plugin as well

### New UI Elements 

- Sliders can now be vertical or horizontal using the "horizontal" property in the JSON
  - comboboxes:
  - getComboBoxSelectionText() of gui_params
  - add in JSON using "comboBoxes" property

### Preset Management
 - new flag added to deploy() to specify new_preset_loaded_since_last_call
 - preset data (defined as map<string, tensor>) is now accessible and updatable via CustomPresetData
 - For documentation, use the deploy() function as an example

### NOW ONLY SINGLE THREAD VERSION

- ONLY DPL THREAD

-----

-----

_______


#### TODOS

- AUDIO_PROCESSOR Thread Implementation
- AUDIO VISUALIZER GUI ELEMENT
- Midi Visualizer GUI Element
