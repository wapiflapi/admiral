#include "plugin.hpp"

using namespace simd;


struct Watches : Module {
    enum ParamIds {
        W11_PARAM,
        W12_PARAM,
        W13_PARAM,
        W14_PARAM,
        W15_PARAM,
        W21_PARAM,
        W22_PARAM,
        W23_PARAM,
        W24_PARAM,
        W25_PARAM,
        BUSCNT_PARAM,
        JOIN_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        W11_INPUT,
        W12_INPUT,
        W13_INPUT,
        W21_INPUT,
        W22_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        W14_OUTPUT,
        W15_OUTPUT,
        W23_OUTPUT,
        W24_OUTPUT,
        W25_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        JOIN_LIGHT,
        NUM_LIGHTS
    };

    bool join = false;
    dsp::BooleanTrigger joinTrigger;

    Watches() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(W11_PARAM, 0.f, 2.f, 1.f, "Jack 1 Bus");
        configParam(W12_PARAM, 0.f, 2.f, 1.f, "Jack 2 Bus");
        configParam(W13_PARAM, 0.f, 2.f, 1.f, "Jack 3 Bus");
        configParam(W14_PARAM, 0.f, 2.f, 1.f, "Jack 4 Bus");
        configParam(W15_PARAM, 0.f, 2.f, 1.f, "Jack 5 Bus");
        configParam(W21_PARAM, 0.f, 2.f, 1.f, "Jack 6 Bus");
        configParam(W22_PARAM, 0.f, 2.f, 1.f, "Jack 7 Bus");
        configParam(W23_PARAM, 0.f, 2.f, 1.f, "Jack 8 Bus");
        configParam(W24_PARAM, 0.f, 2.f, 1.f, "Jack 9 Bus");
        configParam(W25_PARAM, 0.f, 2.f, 1.f, "Jack 10 Bus");

        configParam(BUSCNT_PARAM, 0.f, 2.f, 2.f, "Bus count");
        configParam(JOIN_PARAM, 0.f, 1.f, 1.f, "Join");

    }

    void process(const ProcessArgs &args) override {

        float buscnt = params[BUSCNT_PARAM].getValue();

        for (int bus = 0; bus < 3; ++bus) {
            float bus_select_voltage = float(bus);

            int top_channel_count = 0, bottom_channel_count = 0;

            for (int channel = 0; channel < 16; channel++) {
                float top, bottom;

# define check_channel(DEST, COUNT, PARAM, INPUT)  {                    \
                    if (inputs[INPUT].getChannels() > channel           \
                        && params[PARAM].getValue() == bus_select_voltage) { \
                        COUNT = channel + 1;                            \
                        DEST += inputs[INPUT].getVoltage(channel);      \
                    }                                                   \
                }

                top = 0.f;
                check_channel(top, top_channel_count, W11_PARAM, W11_INPUT);
                check_channel(top, top_channel_count, W12_PARAM, W12_INPUT);
                check_channel(top, top_channel_count, W13_PARAM, W13_INPUT);

                bottom = 0.f;
                check_channel(bottom, bottom_channel_count, W21_PARAM, W21_INPUT);
                check_channel(bottom, bottom_channel_count, W22_PARAM, W22_INPUT);

                if (top_channel_count <= channel && bottom_channel_count <= channel) {
                    break;  // No point in continuing.
                }

                if (bus == 1 && buscnt == 2.0) {
                    // Middle bus needs to mute because we only want two buses.
                    top = bottom = 0.0;
                }

                if (join || (bus == 1 && buscnt == 1.0)) {
                    // We are joining, or middle bus is crossing sections.
                    top = bottom = top + bottom;
                }

                if (params[W14_PARAM].getValue() == bus_select_voltage)
                    outputs[W14_OUTPUT].setVoltage(top, channel);
                if (params[W15_PARAM].getValue() == bus_select_voltage)
                    outputs[W15_OUTPUT].setVoltage(top, channel);

                if (params[W23_PARAM].getValue() == bus_select_voltage)
                    outputs[W23_OUTPUT].setVoltage(bottom, channel);
                if (params[W24_PARAM].getValue() == bus_select_voltage)
                    outputs[W24_OUTPUT].setVoltage(bottom, channel);
                if (params[W25_PARAM].getValue() == bus_select_voltage)
                    outputs[W25_OUTPUT].setVoltage(bottom, channel);

            }

            if (join || (bus == 1 && buscnt == 1.0)) {
                // We are joining, or middle bus is crossing sections.
                // Both halves will have max number of channels.
                if (top_channel_count > bottom_channel_count)
                    bottom_channel_count = top_channel_count;
                else
                    top_channel_count = bottom_channel_count;
            }

            if (params[W14_PARAM].getValue() == bus_select_voltage)
                outputs[W14_OUTPUT].setChannels(top_channel_count);
            if (params[W15_PARAM].getValue() == bus_select_voltage)
                outputs[W15_OUTPUT].setChannels(top_channel_count);

            if (params[W23_PARAM].getValue() == bus_select_voltage)
                outputs[W23_OUTPUT].setChannels(bottom_channel_count);
            if (params[W24_PARAM].getValue() == bus_select_voltage)
                outputs[W24_OUTPUT].setChannels(bottom_channel_count);
            if (params[W25_PARAM].getValue() == bus_select_voltage)
                outputs[W25_OUTPUT].setChannels(bottom_channel_count);


        }

        if (joinTrigger.process(params[JOIN_PARAM].getValue() > 0.f)) {
            join ^= true;
        }

        lights[JOIN_LIGHT].setBrightness(join ? 0.9f : buscnt == 1.0 ? 0.01f : 0.f);

    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "join", json_boolean(join));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        join = json_boolean_value(json_object_get(rootJ, "states"));
    }

};


struct WatchesWidget : ModuleWidget {
    WatchesWidget(Watches *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Watches.svg")));

        addChild(createWidget<ScrewSilver>(Vec(0, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addInput(createInputCentered<PJ301MPort>(Vec(19, 32),  module, Watches::W11_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(19, 62),  module, Watches::W12_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(19, 92),  module, Watches::W13_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(19, 122), module, Watches::W14_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(19, 152), module, Watches::W15_OUTPUT));

        addParam(createParamCentered<CKSSThree>(Vec(19, 182), module, Watches::BUSCNT_PARAM));

        addInput(createInputCentered<PJ301MPort>(Vec(19, 212), module, Watches::W21_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(19, 242), module, Watches::W22_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(19, 272), module, Watches::W23_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(19, 302), module, Watches::W24_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(19, 332), module, Watches::W25_OUTPUT));

        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 32),  module, Watches::W11_PARAM));
        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 62),  module, Watches::W12_PARAM));
        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 92),  module, Watches::W13_PARAM));
        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 122), module, Watches::W14_PARAM));
        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 152), module, Watches::W15_PARAM));

        addParam(createParamCentered<LEDBezel>(Vec(54, 182), module, Watches::JOIN_PARAM));
        addChild(createLightCentered<MuteLight<RedLight>>(Vec(54, 182), module, Watches::JOIN_LIGHT));

        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 212), module, Watches::W21_PARAM));
        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 242), module, Watches::W22_PARAM));
        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 272), module, Watches::W23_PARAM));
        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 302), module, Watches::W24_PARAM));
        addParam(createParamCentered<HorizontalBefacoSwitch>(Vec(54, 332), module, Watches::W25_PARAM));



    }
};


Model *modelWatches = createModel<Watches, WatchesWidget>("Watches");
