#pragma once

#include "al/gamepad/util.h"

class ControllerAppletFunction {
    public:
        // Returns bool to indicate success (user may cancel the controller dialog)
        // Changed from void to match LunaKit implementation
        static bool connectControllerSinglePlay(al::GamePadSystem *);
        static bool connectControllerSeparatePlay(al::GamePadSystem *);
};