#pragma once
#include "Addr.h"
#include "tcpServer.h"
#include <array>
#include <deque>
#include <string>
namespace melonDS { class NDS; }
namespace Plugins::APC {
enum class InstructionSet { Arm, Thumb };
u32 encodeArm9FunctionCall(u32 callAddress, u32 targetAddress, InstructionSet targetInstructionSet);

class Ctx {
public:
    Ctx();
    ~Ctx();
    void setNds(melonDS::NDS* value);
    void setOn(bool value);
    void loadRom();
    void loadSt();
    void commandHandling(u8 opcode, u8 size, u8* buffer);
    void poll();
private:
    std::string messageBytes;
    std::deque<u16> bannerItems;
    std::array<u32, 32> bannerCode{};
    u8 bannerPatchAliveTimeFrames = 0;
    void pollObtainedBanner();
    void installObtainedBannerArm9Patch(u32 keyTable, u32 hudSlot, u32 extraKey, u16 item);
    bool bannerPatchRestoralCheck();
    bool fieldHudReady(u32 hud, u32 panel) const;
    bool originalBannerHudTickLoaded() const;
    struct Chest {
        std::vector<u8> states;
    } chest{};
    enum missionGoal {
        NOT_ACHIEVED = 0,
        ACHIEVED = 1,
        OVERACHIEVED = 2
    };
    enum CharIds {
        ROXAS = 0,
        AXEL = 1,
        XIGBAR = 2,
        SAIX = 3,
        XALDIN = 4,
        SORA = 5,
        DEMYX = 6,
        LARXENE = 7,
        LEXAEUS = 8,
        LUXORD = 9,
        MARLUXIA = 10,
        RIKU = 11,
        VEXEN = 12,
        XEMNAS = 13,
        XION = 14,
        ZEXION = 15,
        MICKEY = 16,
        DONALD = 17,
        GOOFY = 18,
        DWROXAS = 19
    }currentChar = ROXAS;
    daysTCP server{*this};
    const u16 soraFlashbackIds[5] = {39, 47, 53, 56, 61};
    const u16 xionDenyTextEnglish[5] = {78, 111, 46, 46, 46};
    const u16 xionDenyTextFrench[6] = {78, 111,110, 46, 46, 46};
    const u16 xionDenyTextGerman[8] = {78, 101,105,110, 32, 46, 46, 46};
    float hpScalePercent = 100;
    float statScalePercent = 100;
    missionGoal missionResult = NOT_ACHIEVED;
    melonDS::NDS* nds = nullptr;
    u32 magicKeys[15] = {0x05D, 0x05E, 0x05F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x065, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B};
    bool ws = false;
    bool apConnectorEnabled = false;
    u32 missionDone = 0;
    bool finalMissionUnlocked = false;
    void enemyScan();
    void setMultipleMissionLocked(u8 amount, u8* buffer);
    void itemGive(u32 item);
    void pollReplaceChest();
    void pollMissionLocking();
    void RowRelease(u32 rows);
    void pollCalculateScaling();
    void pollDetectOpenChests();
    void getInventory();
    bool setCharacterRandoFlag = false;
    void pollDayLocking();
    void pollSkipCutscene();
    void setCharacterRando(u8 size, u8 *buffer);
    void giveItemType(u16 item);
    void pollGameStart();
    void setMissionLocked(u16 mid, bool hidden);
    void returnToApClient(u8 opcode, u8 size, u8* buffer);
    void giveMagic(u8 magicKey, u8 amount);
    void dayJump();
    void readOption(u32 ptr, u16* opts, u16 length);
    bool compareOption(u16* source, const u16* comp, u16 length);
    void assignMissionCharacter(u16 missionID, u8 character);
    void setCharacterMission(u8 size, u8 * buffer);
    u8 r8(u32 addr) const;
    u16 r16(u32 addr) const;
    u32 r32(u32 addr) const;
    bool w8(u32 addr, u8 val);
    bool w16(u32 addr, u16 val);
    bool w32(u32 addr, u32 val);
    void installArm9Patch(u32 patchAddress, const u32* replacementWords, u32* savedOriginalWords, u32 wordCount);
    void restoreArm9Patch(u32 patchAddress, const u32* savedOriginalWords, u32 wordCount);
    u8 get4(u32 gs, u32 mid);
    static void logmine(const char* fmt, ...);
    void pollRewardsBits();
    void pollChar();
    void pollGate();
    void pollMision();
    void pollActiveFieldGoal();
    void run();
};
}
