#include "plugin.hpp"


struct Shifts : Module {
    enum ParamIds {
        P1_PARAM,
        P2_PARAM,
        P3_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        AB_INPUT,
        A_INPUT,
        B_INPUT,
        P1_INPUT,
        P2_INPUT,
        P3_INPUT,
        T_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        A_OUTPUT,
        B_OUTPUT,
        AB_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    dsp::SchmittTrigger trigger;

    // We need to remember levels because when we don't trigger
    // we might want to keep the old (randomly decided) level.
    float stage1levels[2] = {0.0, 0.0};
    float stage2levels[2][2] = {{0.0, 0.0}, {0.0, 0.0}};
    float stage3levels[2] = {0.0, 0.0};

    Shifts() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(P1_PARAM, -1.f, +1.0,  0.0f);
        configParam(P2_PARAM, -1.f, +1.0, +0.5f);
        configParam(P3_PARAM, -1.f, +1.0,  0.0f);
    }

    void shiftbalance(float levels[2], bool trig, enum ParamIds p, enum InputIds i) {

        float knob = clamp(params[p].getValue() + inputs[i].getVoltage() / 5.f, -1.f, 1.f);

        if (abs(knob) < 0.5f)
            levels[1] = clamp(knob, -0.5f, +0.5f) + 0.5f;
        else if (trig) {
            float threshold = knob < 0 ? -knob - 0.5f : -knob + 1.5f;
            levels[1] = random::uniform() > threshold ? 0.0 : 1.0;
        }

        levels[0] = 1.0f - levels[1];
    }

    void process(const ProcessArgs &args) override {

        float gate = 0.0;

        if (inputs[T_INPUT].isConnected()) {
            gate = inputs[T_INPUT].getVoltage();
        } else if (inputs[AB_INPUT].isConnected()) {
            gate = inputs[AB_INPUT].getVoltage();
        }

        bool trig = trigger.process(gate);

        float Xx[2];
        float ab = inputs[AB_INPUT].getVoltage();

        // Since we're splitting up something into two paths we want
        // to amplify both of them proportionally, clamping at 100%.
        shiftbalance(stage1levels, trig, P1_PARAM, P1_INPUT);
        stage1levels[0] = clamp(stage1levels[0] * 2.0f, 0.0f, 1.0f);
        stage1levels[1] = clamp(stage1levels[1] * 2.0f, 0.0f, 1.0f);

        // Processing A-a and B-b separately during stage 2.
        for (int i = 0; i < 2; ++i) {

            // Either A or B.
            float X = ab * stage1levels[i];
            outputs[A_OUTPUT + i].setVoltage(X);

            // Either a or b.
            float x = inputs[A_INPUT + i].getVoltage();

            // Mix both.
            shiftbalance(stage2levels[i], trig, P2_PARAM, P2_INPUT);
            Xx[i] = X * stage2levels[i][0] + x * stage2levels[i][1];
        }

        shiftbalance(stage3levels, trig, P3_PARAM, P3_INPUT);
        outputs[AB_OUTPUT].setVoltage(
            Xx[0] * stage3levels[0] +
            Xx[1] * stage3levels[1]);

    }
};


struct ShiftsWidget : ModuleWidget {
    ShiftsWidget(Shifts *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Shifts.svg")));

        addChild(createWidget<ScrewSilver>(Vec(0, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        int hinc = 40;
        int height = hinc;

        addInput(createInputCentered<PJ301MPort>(Vec(19, height),  module, Shifts::AB_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(54, height),  module, Shifts::P1_INPUT));

        height += hinc;

        addParam(createParamCentered<Davies1900hWhiteKnob>(Vec(37, height),  module, Shifts::P1_PARAM));

        height += hinc;

        addOutput(createOutputCentered<PJ301MPort>(Vec(19, height), module, Shifts::A_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(54, height), module, Shifts::B_OUTPUT));

        height += hinc;

        addParam(createParamCentered<Davies1900hRedKnob>(Vec(37, height),  module, Shifts::P2_PARAM));

        height += hinc;

        addInput(createInputCentered<PJ301MPort>(Vec(19, height),  module, Shifts::A_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(54, height),  module, Shifts::B_INPUT));

        height += hinc;

        addParam(createParamCentered<Davies1900hWhiteKnob>(Vec(37, height),  module, Shifts::P3_PARAM));

        height += hinc;

        addInput(createInputCentered<PJ301MPort>(Vec(19, height),  module, Shifts::P3_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(54, height), module, Shifts::AB_OUTPUT));

        height += hinc;

        addInput(createInputCentered<PJ301MPort>(Vec(19, height),  module, Shifts::P2_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(54, height),  module, Shifts::T_INPUT));

    }
};


Model *modelShifts = createModel<Shifts, ShiftsWidget>("Shifts");
