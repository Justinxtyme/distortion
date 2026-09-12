/*
==============================================================================

BEGIN_JUCE_MODULE_DECLARATION

   ID:            fuzz_plugin
   vendor:        Bludgeon
   version:       1.0.0
   name:          Fuzz Plugin
   description:   Core of the fuzz plugin
   dependencies:  juce_audio_utils, juce_dsp

   website:
   license:

END_JUCE_MODULE_DECLARATION

==============================================================================
*/

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>
#include <functional>
#include <ranges>
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <span>

#include "include/Fuzz/detail/StridedQueue.h"

#include "include/Fuzz/Parameters.h"
// #include "include/Fuzz/CustomLookAndFeel.h"
#include "include/Fuzz/JsonSerializer.h"
#include "include/Fuzz/LfoVisualizer.h"
#include "include/Fuzz/SampleFifo.h"
#include "include/Fuzz/FuzzEngine.h"
#include "include/Fuzz/BypassTransitionSmoother.h"
#include "include/Fuzz/PluginProcessor.h"
// #include "include/Fuzz/MessageOnClick.h"
#include "include/Fuzz/PluginEditor.h"

