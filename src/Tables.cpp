#include "plugin.hpp"

struct Tables : Module {
    enum ParamIds {
        ENUMS(MOD_PARAM, 8),
        ENUMS(PAT_PARAM, 8),
        SELECT_PARAM,
        ORDER_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CLOCK_INPUT,
        RESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(GATE_OUTPUT, 8),
        ENUMS(TRIG_OUTPUT, 8),
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(MOD_LIGHT, 8),
        ENUMS(PAT_LIGHT, 8),
        ENUMS(STEP_LIGHT, 8 * 4),
        ENUMS(CHANNEL_LIGHT, 4),
        SELECT_LIGHT,
        ORDER_LIGHT,
        NUM_LIGHTS
    };

    int map[16][4] = {
        {0, 0, 0, 0},

        {1, 0, 0, 0},
        {1, 1, 0, 0},
        {1, 1, 1, 0},
        {1, 1, 1, 1},

        {0, 2, 0, 0},
        {0, 0, 3, 0},
        {0, 0, 0, 4},

        {1, 0, 2, 0},
        {0, 2, 1, 0},
        {0, 2, 0, 2},

        {1, 0, 0, 3},
        {0, 0, 3, 1},

        {1, 1, 0, 2},
        {1, 0, 2, 1},
        {0, 2, 1, 1},
    };

    dsp::SchmittTrigger reset_trigger;
    dsp::SchmittTrigger clock_trigger;

    dsp::BooleanTrigger mod_triggers[8];
    dsp::BooleanTrigger pat_triggers[8];

    dsp::BooleanTrigger select_trigger;
    dsp::BooleanTrigger order_trigger;

    struct conf {

        int sel;

        struct channel {
            int order;

            bool reset;
            int beat;
            int beats[4];

            int step;
            struct step {
                int mode;
                int pattern;
            } steps[8];

        } channels[4];
    } conf = {};

    Tables() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < 8; ++i) {
            configParam(MOD_PARAM + i, 0.f, 1.f, 0.f, string::f("Mode %d", i+1));
            configParam(PAT_PARAM + i, 0.f, 1.f, 0.f, string::f("Pattern %d", i+1));
        }

        configParam(SELECT_PARAM, 0.f, 1.f, 1.f, "Select");
        configParam(SELECT_PARAM, 0.f, 1.f, 1.f, "Sequence Mode");

        onReset();
    }

    void onReset() override {
        memset(&conf, 0, sizeof conf);
        for (int c = 0; c < 4; c++) {
            conf.channels[c].reset = true;
        }
    }

    void onRandomize() override {
        onReset();
        for (int c = 0; c < 4; c++) {
            conf.channels[c].order = (int) (4.f * random::uniform());
            for (int i = 0; i < 8; i++) {
                conf.channels[c].steps[i].mode = (int) (4.f * random::uniform());
                conf.channels[c].steps[i].pattern = (((int) (16.f * std::pow(random::uniform(), 3))) + 1) % 16;
            }
        }
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        json_object_set_new(rootJ, "sel", json_integer(conf.sel));

        json_t *channels = json_array();

        for (int c = 0; c < 4; c++) {

            json_t *channel = json_object();

            json_object_set_new(channel, "order", json_integer(conf.channels[c].order));

            json_object_set_new(channel, "beat", json_integer(conf.channels[c].beat));
            json_t *beats = json_array();
            for (int i = 0; i < 4; i++) {
                json_array_insert_new(beats, i, json_integer(conf.channels[c].beats[i]));
            }
            json_object_set_new(channel, "beats", beats);

            json_object_set_new(channel, "step", json_integer(conf.channels[c].step));
            json_t *steps = json_array();
            for (int i = 0; i < 8; i++) {
                json_t *step = json_object();
                json_object_set_new(step, "mode", json_integer(conf.channels[c].steps[i].mode));
                json_object_set_new(step, "pattern", json_integer(conf.channels[c].steps[i].pattern));
                json_array_insert_new(steps, i, step);
            }
            json_object_set_new(channel, "steps", steps);

            json_array_insert_new(channels, c, channel);

        }

        json_object_set_new(rootJ, "channels", channels);

        // json_dumpf(rootJ, stdout, JSON_INDENT(4));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *selJ = json_object_get(rootJ, "sel");
        if (selJ)
            conf.sel = json_integer_value(selJ);

        json_t *channelsJ = json_object_get(rootJ, "channels");
        if (!channelsJ) {
            fprintf(stderr, "Can't load channels.\n");
            return;
        }

        for (int c = 0; c < 4; c++) {
            json_t *channelJ = json_array_get(channelsJ, c);
            if (!channelJ) {
                fprintf(stderr, "Can't load channel (singular).\n");
                continue;
            }

            json_t *orderJ = json_object_get(channelJ, "order");
            if (orderJ)
                conf.channels[c].order = json_integer_value(orderJ);

            json_t *beatJ = json_object_get(channelJ, "beat");
            if (beatJ)
                conf.channels[c].beat = json_integer_value(beatJ);

            json_t *beatsJ = json_object_get(channelJ, "beats");
            if (!beatsJ) {
                fprintf(stderr, "Can't load beats.\n");
                continue;
            }

            for (int i = 0; i < 4; i++) {
                json_t *beatJ = json_array_get(beatsJ, i);
                if (!beatJ) {
                    fprintf(stderr, "Can't load beat (singular).\n");
                    continue;
                }

                conf.channels[c].beats[i] = json_integer_value(beatJ);
            }

            json_t *stepJ = json_object_get(channelJ, "step");
            if (stepJ)
                conf.channels[c].step = json_integer_value(stepJ);

            json_t *stepsJ = json_object_get(channelJ, "steps");
            if (!stepsJ) {
                fprintf(stderr, "Can't load steps.\n");
                continue;
            }
            for (int i = 0; i < 8; i++) {
                json_t *stepJ = json_array_get(stepsJ, i);
                if (!stepJ) {
                    fprintf(stderr, "Can't load step (singular).\n");
                    continue;
                }

                json_t *modeJ = json_object_get(stepJ, "mode");
                if (modeJ)
                    conf.channels[c].steps[i].mode = json_integer_value(modeJ);
                json_t *patternJ = json_object_get(stepJ, "pattern");
                if (patternJ)
                    conf.channels[c].steps[i].pattern = json_integer_value(patternJ);
            }

        }
    }


    bool channelClock(int c) {

        if (conf.channels[c].reset) {
            // This way we'll load the first step again.
            conf.channels[c].step = 7;
            conf.channels[c].beat = 4;
            conf.channels[c].reset = false;
        }

        bool new_step = false;

        // We can scan (lookahead) 8 steps maximum before looping.
        for (int lookahead = 0; lookahead <= 8; ++lookahead) {

            while (++conf.channels[c].beat < 4) {
                if (conf.channels[c].beats[conf.channels[c].beat]) {
                    // This is non-zero so we found something to play.
                    return new_step;
                }
            }

            // We finished the current step, need to load a new one.

            int random_offset = (int) (random::uniform() * 8.0f);
            int offset = 0;

            if (conf.channels[c].order == 0) {
                // forward
                offset = 1;
            } else if (conf.channels[c].order == 1) {
                // random, keep it random.
                offset = random_offset;
            } else if (conf.channels[c].order == 2) {
                // brownian
                offset = random_offset < 4 ? 1 : random_offset < 6 ? 0 : 7;
            } else if (conf.channels[c].order == 3) {
                offset = 7;
            }

            conf.channels[c].step = (conf.channels[c].step + offset) % 8;
            conf.channels[c].beat = -1;
            new_step = true;

            // Initialize the current beat by resolving pattern probabilities.
            int pattern = conf.channels[c].steps[conf.channels[c].step].pattern;
            int mode = conf.channels[c].steps[conf.channels[c].step].mode;

            bool skipping = false;

            for (int i = 0; i < 4; ++i) {

                if (map[pattern][i] == 0) {
                    // Always skip steps exclusively modifying probabilities.
                    conf.channels[c].beats[i] = 0;
                    skipping = false;
                } else if (skipping) {
                    // If we're randomly muting this beat we have to wait
                    // (as opposed to fast-forwarding) hence -1 and not 0.
                    conf.channels[c].beats[i] = -1;
                } else {
                    skipping = random::uniform() > (1.0f / map[pattern][i]);
                    conf.channels[c].beats[i] = skipping ? -1 : 1;
                    // Only skip whole blocks in join mode.
                    skipping = skipping && mode == 3;
                }

            }

            // Finish building the current beat by applying mode logic.

            int last_idx = -1;
            bool first = true;

            for (int i = 0; i < 4; ++i) {

                if (conf.channels[c].beats[i] == -1) {
                    // A muted beat creates a gap, re-setting joins and firsts.
                    if (last_idx >= 0 && conf.channels[c].beats[last_idx] == 2) {
                        // The last beat of a joined note can't be full.
                        conf.channels[c].beats[last_idx] = 1;
                    }
                    last_idx = -1;
                    continue;
                } else if (conf.channels[c].beats[i] == 0) {
                    // This is being fast-forwarded.
                    first = true;
                    continue;
                }

                if (mode == 0) {
                    // Mute: Do nothing during this beat, just wait.
                    conf.channels[c].beats[i] = -1;
                } else if (mode == 1) {
                    // First: Wait unless this is the first beat in a group.
                    conf.channels[c].beats[i] = first ? 1 : -1;
                } else if (mode == 2) {
                    // Pulse: play every beat, this is easy: nothing to change.
                } else if (mode == 3) {
                    // Join: We want to play full beat as long as next is also
                    // playing, but we don't know where next is (could be skip)
                    // For now always play fully and we'll fix last_idx later.
                    conf.channels[c].beats[i] = 2;
                }

                first = false;
                last_idx = i;
            }

            if (last_idx >= 0 && conf.channels[c].beats[last_idx] == 2) {
                // The last beat of a joined note can't be full.
                conf.channels[c].beats[last_idx] = 1;
            }

            // Now that the beats are initialized the next loop iteration
            // should find something to play. :-)

        }

        // We didn't find anything to play :-(
        // We return false because we don't have a new_step.
        return false;
    }


    void process(const ProcessArgs &args) override {

        if (reset_trigger.process(inputs[RESET_INPUT].getVoltage())) {
            for (int c = 0; c < 4; ++c) {
                conf.channels[c].reset = true;
            }
        }

        float gate = inputs[CLOCK_INPUT].getVoltage();
        bool clk = clock_trigger.process(gate);

        for (int c = 0; c < 4; ++c) {

            bool new_step = false;

            if (clk) {
                new_step = channelClock(c);
            }

            if (new_step) {
                outputs[TRIG_OUTPUT + c].setVoltage(gate);
            } else {
                outputs[TRIG_OUTPUT + c].setVoltage(0.f);
            }

            // Set output gate according to current beat.

            if (conf.channels[c].beat >= 0) {

                int beat = conf.channels[c].beats[conf.channels[c].beat % 16];

                if (beat == -1) {
                    outputs[GATE_OUTPUT + c].setVoltage(0.f);
                } else if (beat == 1) {
                    outputs[GATE_OUTPUT + c].setVoltage(gate);
                } else if (beat == 2) {
                    outputs[GATE_OUTPUT + c].setVoltage(10.f);
                }

            }
            // Set channel selection indicator.
            lights[CHANNEL_LIGHT + c].setBrightness(c == conf.sel ? 1.f : 0.f);

        }

        int cur_step = conf.channels[conf.sel].step;
        int cur_beat = conf.channels[conf.sel].beat;

        for (int i = 0; i < 8; ++i) {

            int mode = conf.channels[conf.sel].steps[i].mode;
            int pattern = conf.channels[conf.sel].steps[i].pattern;

            if (mod_triggers[i].process(params[MOD_PARAM + i].getValue())) {
                mode = conf.channels[conf.sel].steps[i].mode = (mode + 1) % 4;
            }

            if (pat_triggers[i].process(params[PAT_PARAM + i].getValue())) {
                pattern = conf.channels[conf.sel].steps[i].pattern = (pattern + 1) % 16;
            }

            float brightness = cur_step == i && outputs[GATE_OUTPUT + conf.sel].getVoltage() ? 1.f : 0.25f;
            lights[MOD_LIGHT + i].setBrightness(mode & 1 ? brightness : 0.f);
            lights[PAT_LIGHT + i].setBrightness(mode & 2 ? brightness : 0.f);

            for (int j = 0; j < 4; ++j) {

                int val = map[pattern][j];

                if (cur_step == i && cur_beat == j && val) {
                    lights[STEP_LIGHT + i * 4 + j].setBrightness(1.f);
                } else {
                    lights[STEP_LIGHT + i * 4 + j].setBrightness(val ? 0.5f / (val * 1.5f) : 0.f);
                }
            }

        }


        lights[SELECT_LIGHT].setBrightness(conf.channels[conf.sel].order & 2 ? 1.f : 0.f);
        if (select_trigger.process(params[SELECT_PARAM].getValue() > 0.f)) {
            conf.sel = (conf.sel + 1) % 4;
        }

        lights[ORDER_LIGHT].setBrightness(conf.channels[conf.sel].order & 1 ? 1.f : 0.f);
        if (order_trigger.process(params[ORDER_PARAM].getValue() > 0.f)) {
            conf.channels[conf.sel].order = (conf.channels[conf.sel].order + 1) % 4;
        }

    }

};


struct TablesWidget : ModuleWidget {
    TablesWidget(Tables *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Tables.svg")));

        addChild(createWidget<ScrewSilver>(Vec(0, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        int height = 35;
        int hinc = 27;
        int winc = 35;

        for (int i = 0; i < 8; ++i) {

            int width = 19;

            addParam(createParamCentered<LEDBezel>(Vec(width, height), module, Tables::MOD_PARAM + i));
            addChild(createLightCentered<MuteLight<WhiteLight>>(Vec(width, height), module, Tables::MOD_LIGHT + i));

            width += winc;

            addParam(createParamCentered<LEDBezel>(Vec(54, height), module, Tables::PAT_PARAM + i));
            addChild(createLightCentered<MuteLight<WhiteLight>>(Vec(width, height), module, Tables::PAT_LIGHT + i));

            width += winc;

            int ledwidth = 18;
            width -= ledwidth / 2;

            for (int j = 0; j < 4; ++j) {
                addChild(createLightCentered<MediumLight<RedLight>>(Vec(width, height), module, Tables::STEP_LIGHT + i * 4 + j));
                width += ledwidth;
            }

            height += hinc;

        }

        int width = 19;

        addParam(createParamCentered<LEDBezel>(Vec(width, height), module, Tables::ORDER_PARAM));
        addChild(createLightCentered<MuteLight<RedLight>>(Vec(width, height), module, Tables::ORDER_LIGHT));

        width += winc;

        addParam(createParamCentered<LEDBezel>(Vec(width, height), module, Tables::SELECT_PARAM));
        addChild(createLightCentered<MuteLight<RedLight>>(Vec(width, height), module, Tables::SELECT_LIGHT));

        width += winc;

        addInput(createInputCentered<PJ301MPort>(Vec(width, height),  module, Tables::RESET_INPUT));

        width += winc;

        addInput(createInputCentered<PJ301MPort>(Vec(width, height),  module, Tables::CLOCK_INPUT));


        height += hinc;

        height += 5;
        width = 35;

        for (int j = 0; j < 4; ++j) {
            addChild(createLightCentered<MediumLight<GreenLight>>(Vec(width, height), module, Tables::CHANNEL_LIGHT + j));
            width += winc;
        }

        height += 15;
        width = 19;

        for (int j = 0; j < 4; ++j) {
            addOutput(createOutputCentered<PJ301MPort>(Vec(width, height), module, Tables::GATE_OUTPUT + j));
            width += winc;
        }

        height += 40;
        width = 19;

        for (int j = 0; j < 4; ++j) {
            addOutput(createOutputCentered<PJ301MPort>(Vec(width, height), module, Tables::TRIG_OUTPUT + j));
            width += winc;
        }




    }
};


Model *modelTables = createModel<Tables, TablesWidget>("Tables");
