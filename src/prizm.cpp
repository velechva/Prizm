/**
 * @author: github.com/velechva
 * 
 * This software is licensed under the GNU General Public License v3.
 * See LICENSE.md for more details
 */

#include <random>

#include "plugin.hpp"
#include "dsp.hpp"
#include "oscillator.hpp"

#define NUM_OSCILLATORS 4

class Prizm : public Module
{
public:
	enum ParamIds
    {
		A_WAVE_SHAPE,
		B_WAVE_SHAPE,
        C_WAVE_SHAPE,
        D_WAVE_SHAPE,
		A_INTENSITY,
        B_INTENSITY,
        C_INTENSITY,
        D_INTENSITY,
		NUM_PARAMS
	};

	enum InputIds
    {
		A_INTENSITY_MOD,
		B_INTENSITY_MOD,
        C_INTENSITY_MOD,
        D_INTENSITY_MOD,
        A_WAVE_SHAPE_MOD,
        B_WAVE_SHAPE_MOD,
        C_WAVE_SHAPE_MOD,
        D_WAVE_SHAPE_MOD,
		V_OCT,
		NUM_INPUTS
	};

	enum OutputIds
    {
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};

	enum LightIds
    {
		NUM_LIGHTS
	};

	Prizm()
    {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(A_WAVE_SHAPE, 0.0, 1.0, 0.0, "A Wave Shape");
		configParam(B_WAVE_SHAPE, 0.0, 1.0, 0.0, "B Wave Shape");
        configParam(C_WAVE_SHAPE, 0.0, 1.0, 0.0, "C Wave Shape");
        configParam(D_WAVE_SHAPE, 0.0, 1.0, 0.0, "B Wave Shape");
        configParam(A_INTENSITY, 0.0, 1.0, 0.0, "A Intensity");
		configParam(B_INTENSITY, 0.0, 1.0, 0.0, "B Intensity");
        configParam(C_INTENSITY, 0.0, 1.0, 0.0, "C Intensity");
        configParam(D_INTENSITY, 0.0, 1.0, 0.0, "D Intensity");
    }

	/**
	 * Process a new sample
	 */
	void process(const ProcessArgs& args) override
    {
		accumulatePhase(args);

		for (size_t i = 0; i < NUM_OSCILLATORS; i++)
        {
			auto oscillator = m_oscillators[i];

			/**
			 * current un-normalized mix value = switch value + cv input value
			 */
			m_mixValues[i] = clamp(
                params[A_INTENSITY + i].getValue() + inputs[A_INTENSITY_MOD + i].getVoltage(),
				0.0f, 1.0f
			);

            float wave_shape_param = clamp(
                params[A_WAVE_SHAPE + i].getValue() + inputs[A_WAVE_SHAPE_MOD + i].getVoltage(),
                0.0f, 1.0f
            );

			m_wavepoints[i] = oscillator.wavepoint(m_phase, wave_shape_param);
		}

        ::dsp::normalize(m_mixValues, NUM_OSCILLATORS);

		for (size_t i = 0; i < NUM_OSCILLATORS; i++)
        {
			m_wavepoints[i] *= m_mixValues[i];
		}

		float output = tanh(::dsp::pythagoras(m_wavepoints, NUM_OSCILLATORS));

		if (outputs[MAIN_OUTPUT].isConnected())
        {
			outputs[MAIN_OUTPUT].setVoltage(5.0f * output);
		}
	}
private:
	float m_phase = 0.0f;

	float m_wavepoints [NUM_OSCILLATORS] = { 0.0f, 0.0f };
	float m_mixValues  [NUM_OSCILLATORS] = { 0.0f, 0.0f };

	WavetableOscillator m_oscillators [NUM_OSCILLATORS] =
    {
		WavetableOscillator(), 
		WavetableOscillator(),
        WavetableOscillator(),
        WavetableOscillator()
	};

	inline hz getFrequency()
    {
		float pitch = inputs[V_OCT].getVoltage();

		return rack::dsp::FREQ_C4 * std::pow(2.0f, pitch);
	}

	inline void accumulatePhase(const ProcessArgs& args)
    {
		m_phase += this->getFrequency() * args.sampleTime;

		if (m_phase >= 0.5f) {
			m_phase -= 1.f;
		}
	}
};

struct PrizmWidget : ModuleWidget
{
	PrizmWidget(Prizm* module)
    {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        addParam(createParam<RoundSmallBlackKnob>(Vec(scale_x * 5.6f, scale_y * 41.7f), module, Prizm::A_WAVE_SHAPE));
        addParam(createParam<RoundSmallBlackKnob>(Vec(scale_x * 5.6f, scale_y * 99.0f), module, Prizm::B_WAVE_SHAPE));
        addParam(createParam<RoundSmallBlackKnob>(Vec(scale_x * 116.0f, scale_y * 41.7f), module, Prizm::C_WAVE_SHAPE));
        addParam(createParam<RoundSmallBlackKnob>(Vec(scale_x * 116.0f, scale_y * 99.0f), module, Prizm::D_WAVE_SHAPE));

        addParam(createParam<RoundSmallBlackKnob>(Vec(scale_x * 5.6f, scale_y * 58.0f), module, Prizm::A_INTENSITY));
        addParam(createParam<RoundSmallBlackKnob>(Vec(scale_x * 5.6f, scale_y * 116.1f), module, Prizm::B_INTENSITY));
        addParam(createParam<RoundSmallBlackKnob>(Vec(scale_x * 116.0f, scale_y * 58.0f), module, Prizm::C_INTENSITY));
        addParam(createParam<RoundSmallBlackKnob>(Vec(scale_x * 116.0f, scale_y * 116.1f), module, Prizm::D_INTENSITY));

        addInput(createInput<PJ301MPort>(Vec(scale_x * 45.4f, scale_y * 40.7f), module, Prizm::A_WAVE_SHAPE_MOD));
        addInput(createInput<PJ301MPort>(Vec(scale_x * 45.4f, scale_y * 97.6f), module, Prizm::B_WAVE_SHAPE_MOD));
        addInput(createInput<PJ301MPort>(Vec(scale_x * 130.0f, scale_y * 97.6f), module, Prizm::C_WAVE_SHAPE_MOD));
        addInput(createInput<PJ301MPort>(Vec(scale_x * 130.0f, scale_y * 97.6f), module, Prizm::D_WAVE_SHAPE_MOD));

        addInput(createInput<PJ301MPort>(Vec(scale_x * 45.4f, scale_y * 57.0f), module, Prizm::A_INTENSITY_MOD));
        addInput(createInput<PJ301MPort>(Vec(scale_x * 45.4f, scale_y * 116.1f), module, Prizm::B_INTENSITY_MOD));
        addInput(createInput<PJ301MPort>(Vec(scale_x * 130.0f, scale_y * 116.1f), module, Prizm::C_INTENSITY_MOD));
        addInput(createInput<PJ301MPort>(Vec(scale_x * 130.0f, scale_y * 116.1f), module, Prizm::D_INTENSITY_MOD));

        addInput(createInput<PJ301MPort>(Vec(scale_x * 210.3f, scale_y * 40.7f), module, Prizm::V_OCT));
        addOutput(createOutput<PJ301MPort>(Vec(scale_x * 210.3f, scale_y * 57.9f), module, Prizm::MAIN_OUTPUT));
	}
private:
    // VCV rack positional units relative to SVG units
    const float scale_x = 2.20f;
    const float scale_y = 2.60f;
};

Model* modelPrizm = createModel<Prizm, PrizmWidget>("Prizm");
