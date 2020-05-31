#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin *pluginInstance;

// Declare each Model, defined in each module source file
extern Model *modelWatches;
extern Model *modelShifts;
extern Model *modelTables;
extern Model *modelDivisions;


#define ADD_SCREW(X, Y) addChild(createWidget<ScrewSilver>(Vec((X), (Y))))
#define ADD_SCREWS() do {                                               \
        ADD_SCREW(0, 0);                                                \
        ADD_SCREW(box.size.x - 1 * RACK_GRID_WIDTH, 0);                 \
        ADD_SCREW(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH);               \
        ADD_SCREW(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH); \
    } while (0);


template <typename BASE>
struct MuteLight : BASE {
    MuteLight() {
        this->box.size = mm2px(Vec(6.f, 6.f));
    }
};

struct HorizontalBefacoSwitch : app::SvgSwitch {
    HorizontalBefacoSwitch() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ComponentLibrary/HorizontalBefacoSwitch_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ComponentLibrary/HorizontalBefacoSwitch_1.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ComponentLibrary/HorizontalBefacoSwitch_2.svg")));
    }
};
