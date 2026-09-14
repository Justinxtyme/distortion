#pragma once


namespace fuzz {

class FuzzEngine {
public:
    FuzzEngine()
    {
        distortion = std::make_unique<FuzzDistortion>();
    }

    juce::dsp::Oversampling<float> oversampler
    {
        2,   // stereo
        2,   // 4x oversampling
        juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR
    };

    bool isOversamplingEnabled = false;


    // ============================================================
    // Lifecycle
    // ============================================================
    void prepare(double sampleRate, int blockSize)
    {

        distortion->prepare(sampleRate, *this);



        sustainSmoothed.reset(sampleRate, 0.02f);      // 20 ms smoothing

        sustainSmoothed.reset(sampleRate, 0.01f);
        sustainSmoothed.setCurrentAndTargetValue(rawSustainValue);

        toneSmoothed.reset(sampleRate, 0.02f);
        outputLevelSmoothed.reset(sampleRate, 0.02f);

        inputMonitor.prepare(sampleRate);
        outputMonitor.prepare(sampleRate);

        // 1. Reset and initialize processing
        oversampler.reset();
        oversampler.initProcessing (static_cast<size_t> (blockSize));

        juce::ignoreUnused(blockSize);
    }

    void reset()
    {

        distortion->resetAll();

        inputMonitor.reset();
        outputMonitor.reset();
    }


    void process(juce::AudioBuffer<float>& buffer) noexcept
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        // ============================
        // Normal (non-oversampled) path
        // ============================
        if (!isOversamplingEnabled)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    float x = buffer.getSample(ch, sample);

                    inputMonitor.push(x);

                    x = distortion->processSample(x);

                    outputMonitor.push(x);

                    buffer.setSample(ch, sample, x);
                }
            }
            return;
        }


        // ============================
        // Oversampled path
        // ============================



        // ============================================
        // 1. Input stage (always normal rate)
        // ============================================
        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float x = buffer.getSample(ch, sample);


                x = distortion->processInputStage(x);


                buffer.setSample(ch, sample, x);
            }
        }

        // ============================================
        // 1. Drive stage (oversampled rate)
        // ============================================

        // Wrap input buffer in an AudioBlock
        const juce::dsp::AudioBlock<float> inputBlock(buffer);

        // Upsample
        const auto oversampledBlock = oversampler.processSamplesUp(inputBlock);

        const size_t osChannels = oversampledBlock.getNumChannels();
        const size_t osSamples  = oversampledBlock.getNumSamples();

        // Process at oversampled rate (Clang‑Tidy safe)
        for (size_t sample = 0; sample < osSamples; ++sample)
        {
            for (size_t ch = 0; ch < osChannels; ++ch)
            {
                float x = oversampledBlock.getSample(ch, sample);
                inputMonitor.push(x);
                x = distortion->processDriveStage(x);
                oversampledBlock.setSample(ch, sample, x);
            }
        }

        // Downsample back into the original buffer
        juce::dsp::AudioBlock<float> outputBlock(buffer);
        oversampler.processSamplesDown(outputBlock);


        // ============================================
        // 1. Tone Stack and Output stage (always normal rate)
        // ============================================

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float x = buffer.getSample(ch, sample);
                x = distortion->processOutputStage(x);

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
        distortion->setGain(1.0f);
        distortion->setDrive(value);
        //clippingStage2.setDrive(value);
        sustainForTrim = value; // store for auto-gai
        distortion->currentSustain = value;
    }


    void setTone(float value) noexcept
    {
        toneSmoothed.setTargetValue(value);
        distortion->setTone(value);
    }


    void setOutputLevel(float value) noexcept
    {
        outputLevelSmoothed.setTargetValue(value);
        distortion->setLevel(value);
    }

    bool initialModeCall = true;

    void setMode(int mode) noexcept
    {

        currentMode = mode;

        switch (currentMode)
        {
            case 0:
                distortion = std::make_unique<FuzzDistortion>();
                break;

            case 1:
                distortion = std::make_unique<FuzzDistortion>();
                break;

            case 2:
                distortion = std::make_unique<FuzzDistortion>();
                break;

            case 3:
                distortion = std::make_unique<FuzzDistortion>();
                break;

            default:
                distortion = std::make_unique<FuzzDistortion>();
                break;
        }
            // Later: switch Triangle / Rams Head / Russian / NYC constants here
    }

    void setOversampling(bool enabled)
    {
        isOversamplingEnabled = enabled;
    }

    bool checkOversampling() const
    {
        return isOversamplingEnabled;
    }





private:

     struct FuzzInputStage {
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



    struct FuzzStage1 {

        void prepare(double sampleRate, FuzzEngine& engine) {
            juce::dsp::ProcessSpec spec
            {
                sampleRate,
                1,
                1
            };

            lowPassFilter.reset();
            lowPassFilter.prepare(spec);

            // Big Muff transistor collector LPF ~1.6 kHz

            if (!engine.isOversamplingEnabled)
            {
                lowPassFilter.coefficients =
                    juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1600.0f);

                return;
            }

            // Process oversampled
            lowPassFilter.coefficients =
                juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1600.0f / 4);
        }


        void reset()
        {
            lowPassFilter.reset();
        }

        void setDrive(float newDrive) noexcept
        {
            // Sustain maps to diode drive
            //drive = newDrive * 1.0f;  // scaling keeps tanh stable
            drive = 15.0f + (newDrive * 20.0f);

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

    struct TubeStage1 {

        void prepare(double sampleRate, FuzzEngine& engine)
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
            drive = 15.0f + (newDrive * 20.0f);

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

    struct FuzzStage2 {

        void prepare(double sampleRate, FuzzEngine& engine)
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
            if (!engine.isOversamplingEnabled)
            {
                lowPassFilter.coefficients =
                    juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1200.0f);

                return;
            }

            // Process oversampled
            lowPassFilter.coefficients =
                juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1200.0f / 4);
        }


        void reset()
        {
            lowPassFilter.reset();
        }

        void setDrive(float newDrive) noexcept
        {
            // Stage 2 is hotter than stage 1
            //drive = newDrive * 1.5f;   // slightly more aggressive scaling
            drive = 15.0f + (newDrive * 20.0f);   // 1.0 → 2.5

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


    struct FuzzToneStage {

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
                juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 5600.0f);

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


    struct FuzzOutputStage {

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

    struct BaseDistortion
    {
        float currentGain { 0.0f };
        float currentSustain { 0.0f };
        float currentTone { 0.0f };
        float currentLevel { 0.0f };

        virtual ~BaseDistortion() = default;
        virtual void prepare(double sampleRate, FuzzEngine& engine) = 0;
        virtual float processInputStage(float x) = 0;
        virtual float processDriveStage(float x) = 0;
        virtual float processOutputStage(float x) = 0;
        virtual float processSample(float x) = 0;
        virtual void resetAll() {}
        virtual void setGain(float gain) noexcept = 0;
        virtual void setDrive(float drive) noexcept = 0;
        virtual void setTone(float tone) noexcept = 0;
        virtual void setLevel(float level) noexcept = 0;
    };

    std::unique_ptr<BaseDistortion> distortion;


    struct FuzzDistortion : BaseDistortion
    {
        FuzzInputStage input;
        FuzzStage1 clip1;
        FuzzStage2 clip2;
        FuzzToneStage toneStack;
        FuzzOutputStage output;

        void prepare(double sampleRate, FuzzEngine& engine) override
        {
            input.prepare(sampleRate);
            clip1.prepare(sampleRate, engine);
            clip2.prepare(sampleRate, engine);
            toneStack.prepare(sampleRate);
            output.prepare(sampleRate);
        }

        float processInputStage(float x) override
        {
            x = input.processSample(x);

            return x;
        }

        float processDriveStage(float x) override
        {
            x = clip1.processSample(x);
            x = clip2.processSample(x);

            return x;
        }

        float processOutputStage(float x) override
        {
            x = toneStack.processSample(x);
            x = output.processSample(x, currentSustain);

            return x;
        }

        float processSample(float x) override
        {
            x = processInputStage(x);
            x = processDriveStage(x);
            x = processOutputStage(x);

            return x;
        }

        void resetAll()  override
        {
            input.reset();
            clip1.reset();
            clip2.reset();
            toneStack.reset();
            output.reset();
        }

        void setGain(float gain) noexcept override
        {
            input.setGain(gain);

        }

        void setDrive(float drive) noexcept override
        {
            clip1.setDrive(drive);
            clip2.setDrive(drive);
        }

        void setTone(float tone) noexcept override
        {
            toneStack.setTone(tone);
        }
        void setLevel(float level) noexcept override
        {
            output.setLevel(level);
        }
    };

    struct TubeDistortion : BaseDistortion
    {
        FuzzInputStage input;
        FuzzStage1 clip1;
        FuzzStage2 clip2;
        FuzzToneStage toneStack;
        FuzzOutputStage output;

        void prepare(double sampleRate, FuzzEngine& engine) override
        {
            input.prepare(sampleRate);
            clip1.prepare(sampleRate, engine);
            clip2.prepare(sampleRate, engine);
            toneStack.prepare(sampleRate);
            output.prepare(sampleRate);
        }

        float processSample(float x) override
        {
            input.processSample(x);
            clip1.processSample(x);
            clip2.processSample(x);
            toneStack.processSample(x);


            output.processSample(x, currentSustain);

            return x;
        }

        void resetAll() override
        {
            input.reset();
            clip1.reset();
            clip2.reset();
            toneStack.reset();
            output.reset();
        }

        void setGain(float gain) noexcept override
        {
            input.setGain(gain);

        }

        void setDrive(float drive) noexcept override
        {
            clip1.setDrive(drive);
            clip2.setDrive(drive);
        }

        void setTone(float tone) noexcept override
        {
            toneStack.setTone(tone);
        }
        void setLevel(float level) noexcept override
        {
            output.setLevel(level);
        }
    };





    // // ============================================================
    // // Module instances
    // // ============================================================
    // FuzzInputStage     inputStage;
    // FuzzStage1  clippingStage1;
    // FuzzStage2  clippingStage2;
    // FuzzToneStage      toneStage;
    // FuzzOutputStage    outputStage;

    // ============================================================
    // Parameter smoothing
    // ============================================================
    juce::SmoothedValue<float> sustainSmoothed;
    juce::SmoothedValue<float> toneSmoothed;
    juce::SmoothedValue<float> outputLevelSmoothed;

    int currentMode { 0 };
    // int lastMode { 0 };

    // ============================================================
    // Monitoring FIFOs
    // ============================================================
    SampleFifo<float> inputMonitor;
    SampleFifo<float> outputMonitor;

};

} // namespace fuzz

