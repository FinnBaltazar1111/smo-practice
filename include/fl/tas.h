#pragma once

#include <sead/math/seadVector.h>
#include <fl/tas2p.h>

namespace fl
{
    struct TasFrame
    {
        sead::Vector2f leftStick = sead::Vector2f(0, 0);
        sead::Vector2f rightStick = sead::Vector2f(0, 0);
        bool A:1, B:1, X:1, Y:1;
        bool L:1, R:1, ZL:1, ZR:1;
        bool plus:1, minus:1, pressLeftStick:1, pressRightStick:1;
        bool dUp:1, dRight:1, dDown:1, dLeft:1;
    };

    struct TasHolder
    {
        static TasHolder& instance() {static TasHolder tasHolder; return tasHolder;}

        // 1-Player TAS data (text format)
        TasFrame* frames = nullptr;

        bool isRunning = false;
        bool startPending = false;
        s64 curFrame = 0;
        u64 frameCount = 0;

        bool oldMotion = false;

        char* scriptName = nullptr;

        // 2-Player TAS data (binary format)
        bool isBinaryFormat = false;          // True if using LunaKit binary format
        bool isTwoPlayer = false;             // True if script uses 2P mode
        Script2P* binaryScript = nullptr;     // Binary format script data
        u32 binaryFrameIndex = 0;             // Current frame index in binary script
        u32 mPrevButtons[2] = {0, 0};         // Previous button state for both players
        u32 gameStep = 0;                     // Game frame counter for step-based playback

        // Methods
        void update();
        void update2P();                      // Step-based update for binary format
        void applyFrame2P(InputFrame2P& frame);  // Apply 2P frame to controllers
        void start();
        void stop();
        void setScriptName(char* name);
        void onStageKill();
    };
}
