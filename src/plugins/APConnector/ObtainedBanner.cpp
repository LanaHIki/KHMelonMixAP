#include "State.h"
#include "itemDatabase.h"
#include <algorithm>

namespace Plugins::APC {
namespace {
constexpr u32 loadFinishedFlag = 0xE59F0074; // LDR r0, [pc, #0x74]
constexpr u32 checkFinishedFlag = 0xE3500000; // CMP r0, #0
}

void Ctx::installObtainedBannerArm9Patch(u32 keyTable, u32 hudSlot, u32 extraKey, u16 item) {
    constexpr u32 returnIfFinished = 0x012FFF1E; // BXEQ lr
    constexpr u32 saveRegisters = 0xE92D5FF0; // PUSH {r4-r12, lr}
    constexpr u32 loadNameTableAddress = 0xE59F4054; // LDR r4, [pc, #0x54]
    constexpr u32 loadHudSlotAddress = 0xE59F5054; // LDR r5, [pc, #0x54]
    constexpr u32 loadExtraKeyAddress = 0xE59F6054; // LDR r6, [pc, #0x54]
    constexpr u32 loadItemKey = 0xE59F7054; // LDR r7, [pc, #0x54]

    // Load the localized item name.
    constexpr u32 saveNameTable = 0xE8940300; // LDMIA r4, {r8, r9}
    constexpr u32 saveExtraKey = 0xE1D6A0B0; // LDRH r10, [r6]
    constexpr u32 writeItemKey = 0xE1C670B0; // STRH r7, [r6]
    constexpr u32 increaseNamedKeyCount = 0xE2890801; // ADD r0, r9, #0x10000
    constexpr u32 writeNamedKeyCount = 0xE5840004; // STR r0, [r4, #4]
    // Ov002_BuildKeyEntryTable() loads the localized names for the field's item keys.
    const u32 buildKeyEntryTable = encodeArm9FunctionCall(BANNERHUDTICK + 13 * 4, BANNERBUILDKEYTABLE, InstructionSet::Thumb);

    // Draw and announce the item.
    constexpr u32 saveOriginalSlotKey = 0xE1D5B0B0; // LDRH r11, [r5]
    constexpr u32 putItemInHudSlot = 0xE1C570B0; // STRH r7, [r5]
    // Ov002_BuildTileRow() draws the item name into the borrowed slot's graphics tiles.
    const u32 buildTileRow = encodeArm9FunctionCall(BANNERHUDTICK + 16 * 4, BANNERBUILDLABELS, InstructionSet::Thumb);
    constexpr u32 passItemKey = 0xE1A00007; // MOV r0, r7
    constexpr u32 enableSound = 0xE3A01001; // MOV r1, #1
    // Ov002_AnnounceWithSound(item, 1) shows the banner and plays its sound.
    const u32 announceWithSound = encodeArm9FunctionCall(BANNERHUDTICK + 19 * 4, BANNERANNOUNCE, InstructionSet::Arm);

    // Restore the borrowed slot and original name table.
    constexpr u32 restoreHudSlot = 0xE1C5B0B0; // STRH r11, [r5]
    constexpr u32 restoreExtraKey = 0xE1C6A0B0; // STRH r10, [r6]
    // Ov002_FreeRootBuffer0x8d14() frees the temporary name table.
    const u32 freeTemporaryNameTable = encodeArm9FunctionCall(BANNERHUDTICK + 22 * 4, BANNERFREEKEYTABLE, InstructionSet::Thumb);
    constexpr u32 restoreNameTable = 0xE8840300; // STMIA r4, {r8, r9}

    // Mark the patch done and return to the game.
    constexpr u32 setReturnValueToZero = 0xE3A00000; // MOV r0, #0
    constexpr u32 markFinished = 0xE58F0010; // STR r0, [pc, #0x10]
    constexpr u32 restoreRegistersAndReturn = 0xE8BD9FF0; // POP {r4-r12, pc}

    // Days runs this in place of its HUD tick. The final five words are data.
    const std::array<u32, 32> bannerPatch = {
        // 1. Run once and read the item key and the addresses it will use.
        loadFinishedFlag, checkFinishedFlag, returnIfFinished, saveRegisters,
        loadNameTableAddress, loadHudSlotAddress, loadExtraKeyAddress, loadItemKey,

        // 2. Add the item key and let Days load its localized name.
        saveNameTable, saveExtraKey, writeItemKey, increaseNamedKeyCount, writeNamedKeyCount,
        buildKeyEntryTable,

        // 3. Save the slot key, draw the item name, and show the banner.
        saveOriginalSlotKey, putItemInHudSlot,
        buildTileRow,
        passItemKey, enableSound,
        announceWithSound,

        // 4. Put the slot and name table back. The drawn label stays visible.
        restoreHudSlot, restoreExtraKey,
        freeTemporaryNameTable,
        restoreNameTable,

        // 5. Tell MelonMix it can restore the original HUD tick.
        setReturnValueToZero, markFinished, restoreRegistersAndReturn,
        keyTable, hudSlot, extraKey, item, 1
    };
    installArm9Patch(BANNERHUDTICK, bannerPatch.data(), bannerCode.data(), bannerCode.size());
}

bool Ctx::bannerPatchRestoralCheck() {
    bool shouldRestore = false;
    if (bannerPatchAliveTimeFrames) {
        bannerPatchAliveTimeFrames--;
        // Check that the first two words in memory still match the installed patch.
        bool patchStillInstalled = (r32(BANNERHUDTICK) == loadFinishedFlag) && (r32(BANNERHUDTICK + 4) == checkFinishedFlag);
        // The patch writes zero to this flag when it finishes running.
        bool gameFinishedPatch = patchStillInstalled && (r32(BANNERHUDTICK + BANNERHUDTICKDONE) == 0);
        bool patchWaitExpired = bannerPatchAliveTimeFrames == 0;
        if (patchStillInstalled && (gameFinishedPatch || patchWaitExpired || !apConnectorEnabled)) shouldRestore = true;
        if (!patchStillInstalled) bannerPatchAliveTimeFrames = 0;
    }
    return shouldRestore;
}

bool Ctx::fieldHudReady(u32 hud, u32 panel) const {
    bool inField = (r32(SCENEID) == 2) && !r32(PENDINGID);
    bool paused = r8(BANNERPAUSEMODE) || r8(BANNERGLOBALPAUSE);
    bool panelBusy = r32(panel + BANNERPANELBUSY);
    bool hudLocked = r32(hud + BANNERLOCKHELD) || r32(hud + BANNERLOCKOWNER);
    return inField && !paused && !panelBusy && !hudLocked;
}

bool Ctx::originalBannerHudTickLoaded() const {
    // Check the first three HUD tick instructions to avoid patching unexpected code.
    constexpr u32 saveHudTickRegisters = 0xE92D4010;
    constexpr u32 loadHudPointerAddress = 0xE59F0074;
    constexpr u32 readHudPointer = 0xE5904000;
    return (r32(BANNERHUDTICK) == saveHudTickRegisters) &&
        (r32(BANNERHUDTICK + 4) == loadHudPointerAddress) &&
        (r32(BANNERHUDTICK + 8) == readHudPointer);
}

void Ctx::pollObtainedBanner() {
    if (bannerPatchRestoralCheck()) {
        restoreArm9Patch(BANNERHUDTICK, bannerCode.data(), bannerCode.size());
        bannerPatchAliveTimeFrames = 0;
    }
    u32 hud = r32(BANNERHUD);
    u32 panel = r32(BANNERPANEL);
    u32 field = r32(BANNERFIELDROOT);
    bool bannerWaiting = apConnectorEnabled && !bannerPatchAliveTimeFrames && !bannerItems.empty();
    bool fieldLoaded = hud && panel && field;
    if (bannerWaiting && fieldLoaded && fieldHudReady(hud, panel) && originalBannerHudTickLoaded()) {
        u16 item = bannerItems.front();
        bannerItems.pop_front();
        //get a used hud slot to borrown and overwrite with our item key
        u32 borrowedHudSlot = 0;
        u16 slotCount = r16(hud + BANNERSLOTCOUNT);
        for (u16 slot = 0; (slotCount <= 24) && (slot < slotCount) && !borrowedHudSlot; slot++) {
            u32 slotAddress = hud + BANNERSLOTKEY + slot * BANNERSLOTSTRIDE;
            if (r16(slotAddress)) borrowedHudSlot = slotAddress;
        }
        u32 keyTable = field + BANNERKEYTABLE;
        u8 namedCount = r8(keyTable + BANNERNAMEDCOUNT);
        bool knownItem = std::any_of(itemDb, itemDb + itemDbSize, [item](const ItemName& entry) {
            return entry.key == item;
        });
        if (borrowedHudSlot && r32(keyTable) && (namedCount < 24) && knownItem) {
            u32 extraKey = keyTable + BANNERNAMEDKEYS + namedCount * 2;
            installObtainedBannerArm9Patch(keyTable, borrowedHudSlot, extraKey, item);
            bannerPatchAliveTimeFrames = 8;
        }
    }
}
}
