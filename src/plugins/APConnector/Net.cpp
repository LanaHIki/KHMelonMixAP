#include "State.h"
namespace Plugins::APC {
Ctx::Ctx() {
#ifdef _WIN32
    WSADATA data{};
    ws = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    ws = true;
#endif
}
Ctx::~Ctx() {
#ifdef _WIN32
    if (ws) WSACleanup();
#endif
}
void Ctx::setOn(bool value) {
    apConnectorEnabled = value;
    if (apConnectorEnabled) {
        server.startTcpServer();
    }
}
void Ctx::loadRom() {
    bannerPatchAliveTimeFrames = 0;
    bannerItems.clear();
    missionDone = 0;
}
void Ctx::loadSt() {
    if (bannerPatchAliveTimeFrames) bannerPatchAliveTimeFrames = 1;
    missionDone = 0;
}
void Ctx::poll() {
    if (nds) pollObtainedBanner();
    if (apConnectorEnabled && nds) {
        run();
    }
}
}
