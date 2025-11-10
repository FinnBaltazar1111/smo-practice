#include "al/util.hpp"
#include "fl/tas.h"
#include "fl/ui/ui.h"
#include "fl/util.h"
#include "fl/game.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/StageScene/ChangeStageInfo.h"
#include "nn/mem.h"
#include "rs/util.hpp"
#include "sead/prim/seadSafeString.h"
#include <fl/packet.h>
#include <mem.h>
#include <nn/init.h>
#include <str.h>

namespace smo {
u32 OutPacketLog::calcLen() const
{
    return strlen(message) + 1;
}

void OutPacketLog::construct(u8* dst) const
{
    *dst = type;
    strcpy((char*)dst + 1, message);
}

void InPacketPlayerScriptInfo::parse(const u8* data, u32 len)
{
    fl::TasHolder& h = fl::TasHolder::instance();
    if (h.isRunning)
        h.stop();

    // Check for format flags in first byte
    if (len > 0) {
        h.isBinaryFormat = data[0] & 0x01;  // Bit 0: binary format flag
        h.isTwoPlayer = data[0] & 0x02;      // Bit 1: 2-player mode flag

        // Rest of data is script name
        if (len > 1) {
            scriptName = (char*)alloc(len);
            fl::memcpy(scriptName, data + 1, len - 1);
            scriptName[len - 1] = '\0';
        } else {
            scriptName = nullptr;
        }
    } else {
        // No data - default to text format
        h.isBinaryFormat = false;
        h.isTwoPlayer = false;
        scriptName = nullptr;
    }
}

void InPacketPlayerScriptInfo::on(Server& server)
{
    fl::TasHolder& h = fl::TasHolder::instance();
    if (h.isRunning)
        h.stop();

    if (scriptName)
        h.setScriptName(scriptName);

    // Clean up old script data based on format
    if (h.isBinaryFormat) {
        // Clean up binary format data
        if (h.binaryScript) {
            dealloc(h.binaryScript);
            h.binaryScript = nullptr;
        }
        h.binaryFrameIndex = 0;
        h.mPrevButtons[0] = 0;
        h.mPrevButtons[1] = 0;
    } else {
        // Clean up text format data
        if (h.frames) {
            dealloc(h.frames);
            h.frames = nullptr;
        }
        h.frameCount = 0;
    }

    h.gameStep = 0;
}

void InPacketPlayerTeleport::parse(const u8* data, u32 len)
{
    pos.x = *(float*)&data[0];
    pos.y = *(float*)&data[4];
    pos.z = *(float*)&data[8];
}

void InPacketPlayerTeleport::on(Server& server)
{
    StageScene* stageScene = fl::ui::PracticeUI::instance().getStageScene();
    if (!stageScene)
        return;
    PlayerActorHakoniwa* player = rs::getPlayerActor(stageScene);
    player->startDemoPuppetable();
    al::setTrans(player, pos);
    player->endDemoPuppetable();
}

void InPacketPlayerGo::parse(const u8* data, u32 len)
{
    scenario = *(signed char*)&data[0];
    u8 stageLength = data[1];
    u8 entranceLength = data[2];

    stageName = (char*)alloc(stageLength + 1);
    fl::memcpy(stageName, &data[3], stageLength);
    stageName[stageLength] = '\0';

    entrance = (char*)alloc(entranceLength + 1);
    fl::memcpy(entrance, &data[3 + stageLength], entranceLength);
    entrance[entranceLength] = '\0';

    startScript = data[len - 1];
}

void InPacketPlayerGo::on(Server& server)
{
    StageScene* stageScene = fl::ui::PracticeUI::instance().getStageScene();
    if (!stageScene) {
        if (entrance)
            free(entrance);
        if (stageName)
            free(stageName);
        return;
    }

    ChangeStageInfo info = ChangeStageInfo(stageScene->mDataHolder, entrance, stageName, false, scenario, { 0 });
    stageScene->mDataHolder->changeNextStage(&info, 0);

    if (entrance)
        free(entrance);
    if (stageName)
        free(stageName);

    if (startScript && fl::TasHolder::instance().frames)
        fl::TasHolder::instance().startPending = true;
}

void InPacketPlayerScriptData::parse(const u8* data, u32 len)
{
    fl::TasHolder& h = fl::TasHolder::instance();
    if (h.isRunning)
        h.stop();

    if (h.isBinaryFormat) {
        // Binary format: allocate/append to Script2P buffer
        if (h.binaryScript == nullptr) {
            // First chunk - contains header and initial frames
            h.binaryScript = (fl::Script2P*)alloc(len);
            fl::memcpy(h.binaryScript, data, len);
        } else {
            // Subsequent chunks - append frames
            size_t oldSize = 284 + (h.binaryScript->mFrameCount * sizeof(fl::InputFrame2P));
            size_t newSize = oldSize + len;
            h.binaryScript = (fl::Script2P*)realloc(h.binaryScript, newSize);
            fl::memcpy(((u8*)h.binaryScript) + oldSize, data, len);

            // Update frame count
            h.binaryScript->mFrameCount += len / sizeof(fl::InputFrame2P);
        }
    } else {
        // Text format: existing logic (unchanged)
        size_t cur = h.frameCount;
        if (h.frames) {
            h.frameCount += len / sizeof(fl::TasFrame);
            h.frames = (fl::TasFrame*)realloc(h.frames, h.frameCount * sizeof(fl::TasFrame));
        } else {
            h.frames = (fl::TasFrame*)alloc(len);
            h.frameCount = len / sizeof(fl::TasFrame);
        }
        fl::memcpy(&h.frames[cur], data, len);
    }
}

void InPacketPlayerScriptData::on(Server& server) { }

void InPacketSelect::parse(const u8* data, u32 len)
{
    option = data[0];
}

void InPacketSelect::on(Server& server)
{
    auto& ui = fl::ui::PracticeUI::instance();
    ui.curLine = option;
}

void InPacketUINavigation::parse(const u8* data, u32 len)
{
    inputMask = *((long*)data);
}

void InPacketUINavigation::on(Server& server)
{
    auto& ui = fl::ui::PracticeUI::instance();
    ext_input |= inputMask;
}

void InPacketPlayerScriptState::parse(const u8* data, u32 len)
{
    state = data[0];
}

void InPacketPlayerScriptState::on(Server& server)
{
    auto& tas = fl::TasHolder::instance();
    if(state == 1) {
        if (tas.frames)
            tas.start();
    } else if(state == 0) {
        tas.stop();
    }
}

void InPacketPlayerSetOptions::parse(const u8* data, u32 len)
{
    if (len < 1) return;

    numOptions = data[0];
    if (numOptions == 0) return;

    options = new Option[numOptions];

    u32 offset = 1;
    for (u8 i = 0; i < numOptions && offset < len; i++) {
        if (offset >= len) break;

        u8 nameLen = data[offset++];
        if (offset + nameLen + 1 > len) break;

        options[i].name = new char[nameLen + 1];
        fl::memcpy(options[i].name, &data[offset], nameLen);
        options[i].name[nameLen] = '\0';
        offset += nameLen;

        options[i].value = data[offset++] != 0;
    }
}

void InPacketPlayerSetOptions::on(Server& server)
{
    auto& ui = fl::ui::PracticeUI::instance();
    auto& uiOptions = ui.options;
    auto& renderer = ui.renderer;

    for (u8 i = 0; i < numOptions; i++) {
        const char* name = options[i].name;
        bool value = options[i].value;

        // Main options
        if (strcmp(name, "teleportEnabled") == 0) uiOptions.teleportEnabled = value;
        else if (strcmp(name, "noclipEnabled") == 0) uiOptions.noclipEnabled = value;
        else if (strcmp(name, "shineRefresh") == 0) uiOptions.shineRefresh = value;
        else if (strcmp(name, "gotShineRefresh") == 0) uiOptions.gotShineRefresh = value;
        else if (strcmp(name, "alwaysWarp") == 0) uiOptions.alwaysWarp = value;
        else if (strcmp(name, "disableAutoSave") == 0) uiOptions.disableAutoSave = value;
        else if (strcmp(name, "skipBowser") == 0) uiOptions.skipBowser = value;
        else if (strcmp(name, "buttonMotionRoll") == 0) uiOptions.buttonMotionRoll = value;
        else if (strcmp(name, "moonJump") == 0) uiOptions.moonJump = value;
        else if (strcmp(name, "loadCurrentFile") == 0) uiOptions.loadCurrentFile = value;
        else if (strcmp(name, "loadFileConfirm") == 0) uiOptions.loadFileConfirm = value;
        else if (strcmp(name, "repeatCapBounce") == 0) uiOptions.repeatCapBounce = value;
        else if (strcmp(name, "repeatRainbowSpin") == 0) uiOptions.repeatRainbowSpin = value;
        else if (strcmp(name, "wallJumpCapBounce") == 0) uiOptions.wallJumpCapBounce = value;
        else if (strcmp(name, "disableCameraVertical") == 0) uiOptions.disableCameraVertical = value;
        else if (strcmp(name, "disableCameraStop") == 0) uiOptions.disableCameraStop = value;
        else if (strcmp(name, "noDamageLife") == 0) uiOptions.noDamageLife = value;
        else if (strcmp(name, "lockHack") == 0) uiOptions.lockHack = value;
        else if (strcmp(name, "lockCarry") == 0) uiOptions.lockCarry = value;
        else if (strcmp(name, "disableShineNumUnlock") == 0) uiOptions.disableShineNumUnlock = value;
        else if (strcmp(name, "showOddSpace") == 0) uiOptions.showOddSpace = value;
        else if (strcmp(name, "disablePuppet") == 0) uiOptions.disablePuppet = value;
        else if (strcmp(name, "overrideBowserHat0") == 0) uiOptions.overrideBowserHat0 = value;
        else if (strcmp(name, "reloadDUP") == 0) uiOptions.reloadDUP = value;
        else if (strcmp(name, "shouldRender") == 0) uiOptions.shouldRender = value;
        else if (strcmp(name, "muteBgm") == 0) uiOptions.muteBgm = value;
        else if (strcmp(name, "pipeMazeOverride") == 0) uiOptions.pipeMazeOverride = value;

        // Renderer options
        else if (strcmp(name, "showPlayer") == 0) renderer.showPlayer = value;
        else if (strcmp(name, "showAxis") == 0) renderer.showAxis = value;
        else if (strcmp(name, "showArea") == 0) renderer.showArea = value;
        else if (strcmp(name, "showAreaPoint") == 0) renderer.showAreaPoint = value;
        else if (strcmp(name, "showAreaGroup") == 0) renderer.showAreaGroup = value;
        else if (strcmp(name, "showHitInfoFloor") == 0) renderer.showHitInfoFloor = value;
        else if (strcmp(name, "showHitInfoWall") == 0) renderer.showHitInfoWall = value;
        else if (strcmp(name, "showHitInfoCeil") == 0) renderer.showHitInfoCeil = value;
        else if (strcmp(name, "showHitInfoArray") == 0) renderer.showHitInfoArray = value;
        else if (strcmp(name, "showCRC") == 0) renderer.showCRC = value;
        else if (strcmp(name, "showHitSensors") == 0) renderer.showHitSensors = value;
    }
}

void InPacketPlayerDoAction::parse(const u8* data, u32 len)
{
    if (len < 1) return;

    u8 nameLen = data[0];
    if (nameLen == 0 || nameLen + 1 > len) return;

    actionName = new char[nameLen + 1];
    fl::memcpy(actionName, &data[1], nameLen);
    actionName[nameLen] = '\0';
}

void InPacketPlayerDoAction::on(Server& server)
{
    if (!actionName) return;

    auto& game = fl::Game::instance();
    StageScene* stageScene = fl::ui::PracticeUI::instance().getStageScene();

    if (!stageScene) return;

    // Function-type actions
    if (strcmp(actionName, "killMario") == 0) {
        game.killMario();
    }
    else if (strcmp(actionName, "damageMario") == 0) {
        game.damageMario(1);
    }
    else if (strcmp(actionName, "lifeUpHeart") == 0) {
        game.lifeUpHeart();
    }
    else if (strcmp(actionName, "healMario") == 0) {
        game.healMario();
    }
    else if (strcmp(actionName, "removeCappy") == 0) {
        game.removeCappy();
    }
    else if (strcmp(actionName, "invincibilityStar") == 0) {
        game.invincibilityStar();
    }
}
}
