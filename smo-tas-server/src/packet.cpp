#include "tas.h"
#include "types.h"
#include <cstring>
#include <packet.h>

#include <arpa/inet.h>
#include <iostream>
#include <unordered_map>

void smo::InPacketLog::parse(const u8* data, u32 len)
{
    type = (LogType) data[0];
    message = std::string(data + 1, data + len);
}

void smo::InPacketLog::on(smo::Client &sender, smo::Server& server)
{
    static const std::unordered_map<LogType, std::string> logTypeStrings =
        { {LogType::Log, "LOG"}, {LogType::Warning, "WARNING"}, {LogType::Error, "ERROR"}, {LogType::Fatal, "FATAL"} };
    
    struct in_addr ip;
    ip.s_addr = sender.addr;
    const char* ipString = inet_ntoa(ip);
    
    if (sender.name.empty())
        std::cout << "[" << logTypeStrings.at(type) << "] <" << ipString << ">: " << message << std::endl;
    else
        std::cout << "[" << logTypeStrings.at(type) << "] <" << sender.name << ">: " << message << std::endl;
}

u32 smo::OutPacketPlayerScriptInfo::calcLen()
{
    return scriptName.size();
}

void smo::OutPacketPlayerScriptInfo::construct(u8* dst)
{
    memcpy(dst, scriptName.data(), scriptName.size());
}

u32 smo::OutPacketPlayerTeleport::calcLen()
{
    return 12;
}

void smo::OutPacketPlayerTeleport::construct(u8* dst)
{
    *((float*) &dst[0]) = pos.x;
    *((float*) &dst[4]) = pos.y;
    *((float*) &dst[8]) = pos.z;
}

u32 smo::OutPacketPlayerGo::calcLen()
{
    return stageName.size() + entrance.size() + 4;
}

void smo::OutPacketPlayerGo::construct(u8* dst)
{
    *(signed char*) &dst[0] = scenario;
    dst[1] = stageName.size();
    dst[2] = entrance.size();
    memcpy(dst + 3, stageName.data(), stageName.size());
    memcpy(dst + stageName.size() + 3, entrance.data(), entrance.size());
    dst[stageName.size() + entrance.size() + 3] = startScript;
}

u32 smo::OutPacketPlayerScriptData::calcLen()
{
    return script.frames.size() * sizeof(fl::TasFrame);
}

void smo::OutPacketPlayerScriptData::construct(u8* dst)
{
    memcpy(dst, script.frames.data(), script.frames.size() * sizeof(fl::TasFrame));
}

u32 smo::OutPacketPlayerSetOptions::calcLen()
{
    // Format: [numOptions:u8] + for each option: [nameLen:u8][name:string][value:bool]
    u32 len = 1; // numOptions byte
    for (const auto& [name, value] : options) {
        len += 1; // nameLen byte
        len += name.size(); // name string
        len += 1; // value bool
    }
    return len;
}

void smo::OutPacketPlayerSetOptions::construct(u8* dst)
{
    u32 offset = 0;
    dst[offset++] = (u8)options.size();

    for (const auto& [name, value] : options) {
        dst[offset++] = (u8)name.size();
        memcpy(dst + offset, name.data(), name.size());
        offset += name.size();
        dst[offset++] = value ? 1 : 0;
    }
}

u32 smo::OutPacketPlayerDoAction::calcLen()
{
    // Format: [nameLen:u8][name:string]
    return 1 + actionName.size();
}

void smo::OutPacketPlayerDoAction::construct(u8* dst)
{
    dst[0] = (u8)actionName.size();
    memcpy(dst + 1, actionName.data(), actionName.size());
}