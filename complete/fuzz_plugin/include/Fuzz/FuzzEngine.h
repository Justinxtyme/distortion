#pragma once


namespace fuzz {

class FuzzEngine {
public:
    FuzzEngine() = default;

    // ============================================================
    // Lifecycle
    // ============================================================
    void prepare(double sampleRate, int blockSize)
    {
        inputStage.prepare(sampleRate);
        clippingStage1.prepare(sampleRate);
        clippingStage2.prepare(sampleRate);
        toneStage.prepare(sampleRate);
        outputStage.prepare(sampleRate);

        sustainSmoothed.reset(sampleRate, 0.02f);      // 20 ms smoothing

        sustainSmoothed.reset(sampleRate, 0.01f);
        sustainSmoothed.setCurrentAndTargetValue(rawSustainValue);

        toneSmoothed.reset(sampleRate, 0.02f);
        outputLevelSmoothed.reset(sampleRate, 0.02f);

        inputMonitor.prepare(sampleRate);
        outputMonitor.prepare(sampleRate);

        juce::ignoreUnused(blockSize);
    }

    void reset()
    {
        inputStage.reset();
        clippingStage1.reset();
        clippingStage2.reset();
        toneStage.reset();
        outputStage.reset();

        inputMonitor.reset();
        outputMonitor.reset();
    }

    // ============================================================
    // Audio processing
    // ============================================================
    void process(juce::AudioBuffer<float>& buffer) noexcept
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float x = buffer.getSample(ch, sample);

                inputMonitor.push(x);

                x = inputStage.processSample(x);
                x = clippingStage1.processSample(x);
                x = clippingStage2.processSample(x);
                x = toneStage.processSample(x);
                x = outputStage.processSample(x, sustainForTrim);

                outputMonitor.push(x);

                buffer.setSample(ch, sample, x);
            }
        }
    }

    float sustainForTrim { 0.0f };
    float rawSustainValue { 0.5f };

    // ============================================================
    // Parameter setters
    // ============================================================
    void setSustain(float value) noexcept
    {
        rawSustainValue = value;
        sustainSmoothed.setTargetValue(value);
        inputStage.setGain(1.0f);
        clippingStage1.setDrive(value);
        clippingStage2.setDrive(value);
        sustainForTrim = value; // store for auto-gai
    }

    void setTone(float value) noexcept
    {
        toneSmoothed.setTargetValue(value);
        toneStage.setTone(value);
    }

    void setOutputLevel(float value) noexcept
    {
        outputLevelSmoothed.setTargetValue(value);
        outputStage.setLevel(value);
    }

    void setMode(int mode) noexcept
    {
        currentMode = mode;
        // Later: switch Triangle / Rams Head / Russian / NYC constants here
    }

private:
    // ============================================================
    // DSP Submodules (empty skeletons)
    // ============================================================
    struct InputStage {
      void prepare(double sampleRate)
      {
          juce::dsp::ProcessSpec spec
          {
            sampleRate,
            1,
            1
          };

          highPassFilter.reset();
          highPassFilter.prepare(spec);

          highPassFilter.coefficients =
              juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 100.0f);
      }

      void reset()
      {
          highPassFilter.reset();
      }

      void setGain(float newGain) noexcept
      {
          gain = newGain;
      }

      float processSample(float x) noexcept
      {
          x *= gain;                     // Sustain gain
          return highPassFilter.processSample(x);  // HPF at ~100 Hz
      }

    private:
        float gain { 1.0f };
        juce::dsp::IIR::Filter<float> highPassFilter;
    };

    struct ClippingStage1 {

        void prepare(double sampleRate)
        {
            juce::dsp::ProcessSpec spec
            {
                sampleRate,
                1,
                1
            };

            lowPassFilter.reset();
            lowPassFilter.prepare(spec);

            // Big Muff transistor collector LPF ~1.6 kHz
            lowPassFilter.coefficients =
                juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1600.0f);
        }

        void reset()
        {
            lowPassFilter.reset();
        }

        void setDrive(float newDrive) noexcept
        {
            // Sustain maps to diode drive
            //drive = newDrive * 1.0f;  // scaling keeps tanh stable
            drive = 15.0f + (newDrive * 3.0f);

        }

        float processSample(float x) noexcept
        {
            // Pre‑gain
            x *= drive;

            // Soft‑diode waveshaping
            x = std::tanh(x);

            // Post‑clipping LPF
            return lowPassFilter.processSample(x);
        }

    private:
        float drive { 1.0f };

        juce::dsp::IIR::Filter<float> lowPassFilter;
    };

    struct ClippingStage2 {

        void prepare(double sampleRate)
        {
            juce::dsp::ProcessSpec spec
            {
              sampleRate,
              1,
              1
            };

            lowPassFilter.reset();
            lowPassFilter.prepare(spec);

            // Slightly darker LPF than stage 1 (~1.2 kHz)
            lowPassFilter.coefficients =
                juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1200.0f);
        }

        void reset()
        {
            lowPassFilter.reset();
        }

        void setDrive(float newDrive) noexcept
        {
            // Stage 2 is hotter than stage 1
            //drive = newDrive * 1.5f;   // slightly more aggressive scaling
            drive = 15.0f + (newDrive * 5.0f);   // 1.0 → 2.5

        }

        float processSample(float x) noexcept
        {
            // Pre‑gain
            x *= drive;

            // Soft‑diode waveshaping
            x = std::tanh(x);

            // Post‑clipping LPF (darker than stage 1)
            return lowPassFilter.processSample(x);
        }

    private:
        float drive { 1.0f };

        juce::dsp::IIR::Filter<float> lowPassFilter;
    };


    struct ToneStage {

        void prepare(double sampleRate)
        {
            juce::dsp::ProcessSpec spec { sampleRate, 1, 1 };

            lowPass.reset();
            highPass.reset();

            lowPass.prepare(spec);
            highPass.prepare(spec);

            // Triangle Big Muff tone stack approximations:
            // LPF ~3.4 kHz
            lowPass.coefficients =
                juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 6400.0f);

            // HPF ~500 Hz
            highPass.coefficients =
                juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 500.0f);
        }

        void reset()
        {
            lowPass.reset();
            highPass.reset();
        }

        void setTone(float newTone) noexcept
        {
            // Tone is 0 → dark (LPF)
            // Tone is 1 → bright (HPF)
            tone = newTone;
        }

        float processSample(float x) noexcept
        {
            const float low  = lowPass.processSample(x);
            const float high = highPass.processSample(x);

            // Crossfade between LPF and HPF
            return low * (1.0f - tone) + high * tone;
        }

    private:
        float tone { 0.5f }; // midpoint = classic scoop

        juce::dsp::IIR::Filter<float> lowPass;
        juce::dsp::IIR::Filter<float> highPass;
    };


    struct OutputStage {

        void prepare(double /*sampleRate*/)
        {
          // Nothing to prepare — pure gain stage
        }

        void reset()
        {
          // No state to reset
        }

        void setLevel(float newLevel) noexcept
        {
            level = newLevel;  // linear gain
        }


        float processSample(float x, float sustain) noexcept
        {
            constexpr float k = 0.1f; // gentle compensation
            float masterTrim = 1.0f / (1.0f + sustain * k);
            //float masterTrim = 1.0f - (sustain * 0.10f);


            x *= masterTrim / 10.0f;
            return x * level;
        }


    private:
        float level { 1.0f }; // linear gain
        // masterTrim { 0.25f }; // -12 dB
    };



    // ============================================================
    // Module instances
    // ============================================================
    InputStage     inputStage;
    ClippingStage1  clippingStage1;
    ClippingStage2  clippingStage2;
    ToneStage      toneStage;
    OutputStage    outputStage;

    // ============================================================
    // Parameter smoothing
    // ============================================================
    juce::SmoothedValue<float> sustainSmoothed;
    juce::SmoothedValue<float> toneSmoothed;
    juce::SmoothedValue<float> outputLevelSmoothed;

    int currentMode { 0 };

    // ============================================================
    // Monitoring FIFOs
    // ============================================================
    SampleFifo<float> inputMonitor;
    SampleFifo<float> outputMonitor;

};

} // namespace fuzz

