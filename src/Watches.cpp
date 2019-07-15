#include "plugin.hpp"

template <typename BASE>
struct MuteLight : BASE {
    MuteLight() {
        this->box.size = mm2px(Vec(6.f, 6.f));
    }
};


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
        MUTES_PARAM,
        JOIN_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        W11_INPUT,
        W12_INPUT,
        W13_INPUT,
        W21_INPUT,
        W22_INPUT,
        W23_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        W14_OUTPUT,
        W15_OUTPUT,
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
        configParam(W11_PARAM, 0.f, 2.f, 0.f, "Input 1 Bus");
        configParam(W12_PARAM, 0.f, 2.f, 0.f, "Input 2 Bus");
        configParam(W13_PARAM, 0.f, 2.f, 0.f, "Input 3 Bus");
        configParam(W14_PARAM, 0.f, 2.f, 0.f, "Input 4 Bus");
        configParam(W15_PARAM, 0.f, 2.f, 0.f, "Input 5 Bus");
        configParam(W21_PARAM, 0.f, 2.f, 0.f, "Input 6 Bus");
        configParam(W22_PARAM, 0.f, 2.f, 0.f, "Input 7 Bus");
        configParam(W23_PARAM, 0.f, 2.f, 0.f, "Input 8 Bus");
        configParam(W24_PARAM, 0.f, 2.f, 0.f, "Input 9 Bus");
        configParam(W25_PARAM, 0.f, 2.f, 0.f, "Input 10 Bus");

        configParam(MUTES_PARAM, 0.f, 1.f, 0.f, "Join");
        configParam(JOIN_PARAM, 0.f, 1.f, 1.f, "Mutes");

    }

    void process(const ProcessArgs &args) override {

        bool mute = params[MUTES_PARAM].getValue();

        for (int bus = 0; bus < 3; ++bus) {
            float voltage = float(bus);
            float w1 = 0.f;

            if (params[W11_PARAM].getValue() == voltage)
                w1 += inputs[W11_INPUT].getVoltage();
            if (params[W12_PARAM].getValue() == voltage)
                w1 += inputs[W12_INPUT].getVoltage();
            if (params[W13_PARAM].getValue() == voltage)
                w1 += inputs[W13_INPUT].getVoltage();

            float w2 = 0.f;

            if (params[W21_PARAM].getValue() == voltage)
                w2 += inputs[W21_INPUT].getVoltage();
            if (params[W22_PARAM].getValue() == voltage)
                w2 += inputs[W22_INPUT].getVoltage();
            if (params[W23_PARAM].getValue() == voltage)
                w2 += inputs[W23_INPUT].getVoltage();

            if (bus == 1 && mute) {
                w1 = w2 = 0.0;
            }

            if (join) {
                w1 = w2 = w1 + w2;
            }

            if (params[W14_PARAM].getValue() == voltage)
                outputs[W14_OUTPUT].setVoltage(w1);
            if (params[W15_PARAM].getValue() == voltage)
                outputs[W15_OUTPUT].setVoltage(w1);

            if (params[W24_PARAM].getValue() == voltage)
                outputs[W24_OUTPUT].setVoltage(w2);
            if (params[W25_PARAM].getValue() == voltage)
                outputs[W25_OUTPUT].setVoltage(w2);

        }


        if (joinTrigger.process(params[JOIN_PARAM].getValue() > 0.f)) {
            join ^= true;
        }

        lights[JOIN_LIGHT].setBrightness(join ? 0.9f : 0.f);
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

        addParam(createParamCentered<CKSS>(Vec(19, 182), module, Watches::MUTES_PARAM));

        addInput(createInputCentered<PJ301MPort>(Vec(19, 212), module, Watches::W21_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(19, 242), module, Watches::W22_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(19, 272), module, Watches::W23_INPUT));
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
