#include "plugin.hpp"


struct Divisions : Module {
    enum ParamIds {
        ENUMS(BUS_PARAM, 18),
        ENUMS(DIV_PARAM, 4),
        GATE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CLK_INPUT,
        RST_INPUT,
        GATE_INPUT,
        ENUMS(AUX_INPUT, 2),
        ENUMS(DIV_INPUT, 4),
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(MIX_OUTPUT, 3),
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(BUS_LIGHT, 18),
        ENUMS(DIV_LIGHT, 4 * 16),
        GATE_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger rst_trigger;
    dsp::SchmittTrigger clk_trigger;
    dsp::SchmittTrigger aux_trigger[2];
    dsp::BooleanTrigger bus_triggers[18];

    dsp::PulseGenerator bus_generator[2];
    dsp::PulseGenerator bus_light_generator[2];
    dsp::PulseGenerator light_generator[6];
    dsp::PulseGenerator warning_generator;

    struct conf {
        unsigned int counter;
        bool bus_select[18];
    } conf = {};

    Divisions() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < 2; ++i) {
            configParam(BUS_PARAM + 2 * i, 0.f, 1.f, 0.f,
                        string::f("AUX %d to LEFT bus", i + 1));
            configParam(BUS_PARAM + 2 * i + 1, 0.f, 1.f, 0.f,
                        string::f("AUX %d to RIGHT bus", i + 1));
        }

        for (int i = 2; i < 6; ++i) {
            configParam(BUS_PARAM + 2 * i, 0.f, 1.f, 0.f,
                        string::f("CLK DIV %d to LEFT bus", i - 2 + 1));
            configParam(BUS_PARAM + 2 * i + 1, 0.f, 1.f, 0.f,
                        string::f("CLK DIV %d to RIGHT bus", i - 2 + 1));
        }

        for (int i = 6; i < 9; ++i) {
            configParam(BUS_PARAM + 2 * i, 0.f, 1.f, 0.f,
                        string::f("OUT %d from RIGHT bus", i - 6 + 1));
            configParam(BUS_PARAM + 2 * i + 1, 0.f, 1.f, 0.f,
                        string::f("OUT %d from RIGHT bus", i - 6 + 1));
        }

        for (int i = 0; i < 4; ++i) {
            configParam(DIV_PARAM + i, 0.f, 15.f, 1.f,
                        string::f("CLK DIV %d", i + 1), " steps", 0.f, 1.f, 1.f);
        }

        configParam(GATE_PARAM, 0.f, 1.f, 0.5f,
                    "Gate Length", " s", 10.f / 1e-3, 1e-3);

    }

    void onReset() override {
        memset(&conf, 0, sizeof conf);
    }

    void onRandomize() override {
        onReset();
        for (int c = 0; c < 18; c++) {
            conf.bus_select[c] = random::uniform() < 0.5;
        }
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        json_object_set_new(rootJ, "counter", json_integer(conf.counter));

        json_t *bus_select = json_array();

        for (int c = 0; c < 18; c++) {
            json_array_insert_new(bus_select, c, json_integer(conf.bus_select[c]));
        }

        json_object_set_new(rootJ, "bus_select", bus_select);

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *counterJ = json_object_get(rootJ, "counter");
        if (counterJ)
            conf.counter = json_integer_value(counterJ);

        json_t *bus_selectJ = json_object_get(rootJ, "bus_select");
        if (!bus_selectJ) {
            fprintf(stderr, "Can't load bus_select.\n");
            return;
        }

        for (int i = 0; i < 18; i++) {
            json_t *bsJ = json_array_get(bus_selectJ, i);
            if (!bsJ) {
                fprintf(stderr, "Can't load bus_select (singular).\n");
                continue;
            }
            conf.bus_select[i] = json_integer_value(bsJ);
        }

    }

    void process(const ProcessArgs& args) override {
        bool clk = clk_trigger.process(inputs[CLK_INPUT].getVoltage());
        bool bus_high[2];
        bool bus_light_high[2];
        bool bus_triggered[2];

        if (clk) {
            conf.counter += 1;
        }

        if (rst_trigger.process(inputs[RST_INPUT].getVoltage())) {
            conf.counter = 0;
        }

        if (warning_generator.process(APP->engine->getSampleTime())) {
            lights[GATE_LIGHT].setBrightness(1.0);
        } else {
            lights[GATE_LIGHT].setBrightness(0.0);
        }

        for (int bus = 0; bus < 2; ++bus) {
            bus_high[bus] = bus_generator[bus].process(APP->engine->getSampleTime());
            bus_light_high[bus] = bus_light_generator[bus].process(APP->engine->getSampleTime());
            bus_triggered[bus] = false;
        }

        float gate_mod = (
            inputs[GATE_INPUT].isConnected()
            ? inputs[GATE_INPUT].getVoltage() / 10.0f
            : 1.0);

        float gatelength = std::max(
            1e-3f, // Minimum trigger length.
            params[GATE_PARAM].getValue() * gate_mod);

        float lightgatelength = std::max(
            1.f / 25, // Minimum visible light length.
            gatelength);

        for (int i = 0; i < 6; ++i) {

            bool trigger = false;

            if (i < 2) {
                trigger = aux_trigger[i].process(inputs[AUX_INPUT + i].getVoltage());
            } else {

                float div_mod = std::abs(
                    inputs[DIV_INPUT + (i - 2)].isConnected()
                    ? inputs[DIV_INPUT + (i - 2)].getVoltage() / 10.0f
                    : 1.0);

                unsigned int limit = params[DIV_PARAM + (i - 2)].getValue();
                unsigned int selected = limit * div_mod;
                unsigned int current = conf.counter % (selected + 1);

                for (unsigned int li = 0; li < 16; ++li) {
                    lights[DIV_LIGHT + (i - 2) * 16 + li].setBrightness(
                        li == current ? 1.0 :
                        li == limit ? 0.8 :
                        li >= selected && li < limit ? 0.3 :
                        0.0);
                }

                if (clk && current == selected) {
                    trigger = true;
                }

            }

            if (trigger) {
                light_generator[i].reset();
                light_generator[i].trigger(lightgatelength);
            }

            bool light_high = light_generator[i].process(APP->engine->getSampleTime());

            for (int bus = 0; bus < 2; ++bus) {

                if (bus_triggers[i * 2 + bus].process(
                        params[BUS_PARAM + i * 2 + bus].getValue())) {
                    conf.bus_select[i * 2 + bus] ^= true;
                }

                lights[BUS_LIGHT + i * 2 + bus].setBrightness(
                    conf.bus_select[i * 2 + bus]
                    ? (light_high ? 1.0 : 0.3)
                    : 0.0);

                if (trigger && conf.bus_select[i * 2 + bus]) {
                    bus_triggered[bus] = true;
                    bus_generator[bus].reset();
                    bus_generator[bus].trigger(gatelength);
                    bus_light_generator[bus].reset();
                    bus_light_generator[bus].trigger(lightgatelength);
                }

            }

        }

        for (int i = 0; i < 3; ++i) {
            bool high = false;

            for (int bus = 0; bus < 2; ++bus) {

                if (bus_triggers[(6 + i) * 2 + bus].process(
                        params[BUS_PARAM + (6 + i) * 2 + bus].getValue())) {
                    conf.bus_select[(6 + i) * 2 + bus] ^= true;
                }

                lights[BUS_LIGHT + (6 + i) * 2 + bus].setBrightness(
                    conf.bus_select[(6 + i) * 2 + bus]
                    ? (bus_light_high[bus] ? 1.0 : 0.3)
                    : 0.0);

                if (conf.bus_select[(6 + i) * 2 + bus]) {
                    if (bus_high[bus] && bus_triggered[bus]) {
                        // This means it has triggered but it was
                        // already high. Since we actually use this
                        // it's important!
                        warning_generator.reset();
                        warning_generator.trigger(lightgatelength);
                    }

                    high |= bus_high[bus];
                }

            }

            outputs[MIX_OUTPUT + i].setVoltage(high ? 10.0 : 0.0);
        }


    }

};

struct DivisionsWidget : ModuleWidget {
    DivisionsWidget(Divisions* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Divisions.svg")));
        ADD_SCREWS();

        int height = 33;
        int hinc = 37;
        int winc = 35;

        for (int i = 0; i < 9; ++i) {

            int width = 22;

            if (i == 0) {
                addInput(createInputCentered<PJ301MPort>(
                             Vec(width, height), module,
                             Divisions::CLK_INPUT));
            } else if (i == 1) {
                addInput(createInputCentered<PJ301MPort>(
                             Vec(width, height), module,
                             Divisions::RST_INPUT));
            } else if (i < 6) {
                addInput(createInputCentered<PJ301MPort>(
                             Vec(width, height), module,
                             Divisions::DIV_INPUT + i - 2));
            } else if (i == 6) {
                addInput(createInputCentered<PJ301MPort>(
                             Vec(width, height), module,
                             Divisions::GATE_INPUT));
            } else if (i == 7) {
                addParam(createLightParamCentered<LEDLightSlider<RedLight>>(
                             Vec(width, height + hinc / 2.f), module,
                             Divisions::GATE_PARAM,
                             Divisions::GATE_LIGHT));
            }

            width += winc;

            if (i < 2) {
                addInput(createInputCentered<PJ301MPort>(
                             Vec(width, height), module,
                             Divisions::AUX_INPUT + i));
            } else if (i < 6) {

                for (int ii = 0; ii < 16; ++ii) {
                    float r = 15.0;

                    float a = 0.72;
                    float b = 0.14;

                    float h = r * std::cos(2 * M_PI * (a * (float) ii / 15.0 + b));
                    float w = r * std::sin(2 * M_PI * (a * (float) ii / 15.0 + b));

                    addChild(createLightCentered<TinyLight<RedLight>>(
                                 Vec(width - w, height + h), module,
                                 Divisions::DIV_LIGHT + (i - 2) * 16 + ii));

                }

                addParam(createParamCentered<BefacoTinyKnob>(
                             Vec(width, height), module,
                             Divisions::DIV_PARAM + i - 2));

            } else {
                addOutput(createOutputCentered<PJ301MPort>(
                              Vec(width, height), module,
                              Divisions::MIX_OUTPUT + i - 6));
            }

            width += winc;

            addParam(createParamCentered<LEDBezel>(
                         Vec(width, height), module,
                         Divisions::BUS_PARAM + 2 * i));
            addChild(createLightCentered<MuteLight<WhiteLight>>(
                         Vec(width, height), module,
                         Divisions::BUS_LIGHT + 2 * i));

            width += winc;

            addParam(createParamCentered<LEDBezel>(
                         Vec(width, height), module,
                         Divisions::BUS_PARAM + 2 * i + 1));
            addChild(createLightCentered<MuteLight<WhiteLight>>(
                         Vec(width, height), module,
                         Divisions::BUS_LIGHT + 2 * i + 1));

            height += hinc;

        }

    }
};


Model *modelDivisions = createModel<Divisions, DivisionsWidget>("Divisions");
