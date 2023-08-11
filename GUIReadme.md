## NeuralMidiFX GUI Creation

Creating custom Graphic User Interfaces (GUIs) that are safely 
integrated with the various threads is a time consuming process. We have therefor
developed a system that lets you quickly detail exactly what is needed, and the 
plugin handles the rest. 

NeuralMidiFXPlugin/Source/DeploymentSettings/GuiAndParams.h

In this file you can set your specifications. The model interface is divided into
tabs, each of which is represented as a **tab_tuple** object. For each tab that
you want to create, the following information is needed:

```asm
tab_tuple{
    "tab name",
    slider_list {
        # vertical sliders
        },
    rotary_list {
         # rotary knobs
         },
     button_list {
         # buttons
         }
    };
```

Each slider, rotary and button is expressed as a tuple with the following arguments:
```
slider_tuple{"Name", min_value, max_value, default_value}
rotary_tuple{"Name", min_value, max_value, default_value}
button_tuple{"Name", IsToggleable}
```

The plugin will automatically determine the vertical and horizontal
spacing of the various parameters in relation to each other, while maintaining
a resizeable window. Each parameter will be fully connected to the APVTS and other 
threads, which can be accessed via processes detailed int he following sections. 
We provide a further reference below of a working example of the GuiAndParams.h file,
which results in 3 tabs of various parameters:

```asm
namespace Tabs {
        const std::vector<tab_tuple> tabList{

                tab_tuple{
                        "Model",
                        slider_list{
                                slider_tuple{"Slider 1", 0.0, 1.0, 0.0},
                                slider_tuple{"Slider 2", 0.0, 25.0, 11.0}},
                        rotary_list{
                                rotary_tuple{"Rotary 1", 0.0, 1.0, 0.5},
                                rotary_tuple{"Rotary 2", 0.0, 4.0, 1.5},
                                rotary_tuple{"Rotary 3", 0.0, 1.0, 0.0}},
                        button_list{
                                button_tuple{"ToggleButton 1", true},
                                button_tuple{"TriggerButton 1", false}}
                },

                tab_tuple{
                        "Settings",
                        slider_list{
                                slider_tuple{"Test 1", 0.0, 1.0, 0.0},
                                slider_tuple{"Gibberish", 0.0, 25.0, 11.0}},
                        rotary_list{
                                rotary_tuple{"Rotary 1B", 0.0, 1.0, 0.5},
                                rotary_tuple{"Test 2B", 0.0, 4.0, 1.5}},
                        button_list{
                                button_tuple{"ToggleButton 2", true},
                                button_tuple{"ToggleButton 3", true},
                                button_tuple{"Dynamics", false}}
                },

                tab_tuple{
                        "Generation",
                        slider_list{
                                slider_tuple{"Generation Playback Delay", 0.0, 10.0, 0.0}},
                        rotary_list{},
                        button_list{}
                },
        };
    }
```

