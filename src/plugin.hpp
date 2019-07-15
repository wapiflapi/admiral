#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin *pluginInstance;

// Declare each Model, defined in each module source file
extern Model *modelWatches;

struct HorizontalBefacoSwitch : app::SvgSwitch {
    HorizontalBefacoSwitch() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ComponentLibrary/HorizontalBefacoSwitch_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ComponentLibrary/HorizontalBefacoSwitch_1.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ComponentLibrary/HorizontalBefacoSwitch_2.svg")));
    }
};
