#include <fl/tas.h>
#include <fl/ui/ui.h>
#include <mem.h>
#include <game/Controller/ControllerAppletFunction.h>
#include <game/System/GameSystem.h>
#include <rs/util.hpp>

void fl::TasHolder::update()
{
    if(!isRunning) return;

    // Dispatch to correct update function based on format
    if (isBinaryFormat) {
        update2P();  // Step-based, multi-player
    } else {
        // Existing 1P logic (unchanged)
        if (curFrame + 1 >= frameCount && curFrame >= 0) stop();
        curFrame++;
    }

    // Increment game step counter for both formats
    gameStep++;
}

void fl::TasHolder::onStageKill()
{
    if(startPending) {
        start();
        curFrame = -1;
    }
}

void fl::TasHolder::start()
{
    startPending = false;
    curFrame = 0;
    binaryFrameIndex = 0;
    gameStep = 0;

    // Handle 2P mode switching for binary format scripts
    if (isBinaryFormat && binaryScript) {
        // Get current scene and game pad system
        StageScene* scene = fl::ui::PracticeUI::instance().getStageScene();
        if (scene) {
            // Check if script's 2P mode matches current mode
            if (binaryScript->mIsTwoPlayer != rs::isSeparatePlay(scene)) {
                al::GamePadSystem* gamePadSystem = GameSystemFunction::getGameSystem()->mGamePadSystem;

                if (binaryScript->mIsTwoPlayer) {
                    // Switch TO 2-player mode
                    if (!ControllerAppletFunction::connectControllerSeparatePlay(gamePadSystem)) {
                        return;  // User cancelled controller dialog
                    }
                    rs::changeSeparatePlayMode(scene, true);
                    // TODO: May need to wait 1 frame for controller reconnection
                } else {
                    // Switch TO 1-player mode
                    if (!ControllerAppletFunction::connectControllerSinglePlay(gamePadSystem)) {
                        return;  // User cancelled
                    }
                    rs::changeSeparatePlayMode(scene, false);
                }
            }
        }

        // TODO: Handle stage change metadata if mChangeStageName is not empty
        // This would use ChangeStageInfo like LunaKit does
    }

    isRunning = true;
}

void fl::TasHolder::stop()
{
    startPending = false;
    curFrame = 0;
    isRunning = false;
}

void fl::TasHolder::setScriptName(char* name)
{
    if (scriptName) dealloc(scriptName);
    scriptName = name;
}
