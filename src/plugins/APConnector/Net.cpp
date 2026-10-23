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
    on = value;
    if (on) {
        server.startTcpServer();
    }
}
void Ctx::loadRom() {
    bannerActiveHud = 0;
    obtainedBanners.clear();
    missionDone = 0;
}
void Ctx::loadSt() {
    bannerWaitFrames = 8;
    missionDone = 0;
}
void Ctx::poll() {
    if (nds)
        pollObtainedBanner();
    if (on && nds) {
        run();
    }
}
}
