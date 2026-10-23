#include "State.h"
bool Plugins::APC::Ctx::showObtainedBanner(std::u16string_view text) {
    bool accepted = !text.empty() && text.size() < 32 && text.find(u'\0') == text.npos;
    if (accepted)
        obtainedBanners.emplace_back(text);
    return accepted;
}
bool Plugins::APC::Ctx::startObtainedBanner(std::u16string_view text) {
    bool started = false;
    u32 fieldRoot = r32(MISSIONROOT);
    u32 hud = r32(BANNERHUD);
    u32 panel = r32(BANNERPANEL);
    u32 textEntry = 0;
    bool playableField = fieldRoot && hud && panel && r32(SCENEID) == 2 && !r32(PENDINGID);
    bool idleHud = playableField && !r8(BANNERPAUSEMODE) && !r8(BANNERGLOBALPAUSE) && !r32(panel + BANNERPANELBUSY) && !r32(hud + BANNERLOCKOWNER) && !r32(hud + BANNERLOCKHELD);
    bannerReadyFrames = idleHud ? bannerReadyFrames + 1 : 0;
    bool originalCode = r32(BANNERHUDTICK) == 0xE92D4010 && r32(BANNERHUDTICK + 4) == 0xE59F0074 && r32(BANNERHUDTICK + 8) == 0xE5904000;
    if (bannerReadyFrames >= 30 && originalCode && r32(BANNERGAMECODE) == 0x50474B59) {
        for (u32 index = 0; index < r8(fieldRoot + BANNERKEYCOUNT); index++) {
            u32 entry = r32(fieldRoot + BANNERKEYTABLE) + index * BANNERKEYSTRIDE;
            if (r16(entry + BANNERKEYID) == 2)
                textEntry = entry;
        }
        if (textEntry) {
            bannerActiveHud = hud;
            bannerTextEntry = textEntry;
            bannerFieldRoot = fieldRoot;
            bannerWaitFrames = 0;
            bannerDisplayed = false;
            bannerReadyFrames = 0;
            for (u32 index = 0; index < bannerSavedText.size(); index++)
                bannerSavedText[index] = r8(textEntry + index);
            for (u32 index = 0; index < bannerSavedCode.size(); index++)
                bannerSavedCode[index] = r32(BANNERHUDTICK + index * 4);
            for (u32 index = 0; index < 32; index++)
                w16(textEntry + index * 2, index < text.size() ? text[index] : 0);
            const u32 refreshAndAnnounce[] = {0xE92D4010, 0xE59FC01C, 0xE12FFF3C, 0xE3A00002, 0xE3A01001, 0xE59FC010, 0xE12FFF3C, 0xE3A00000, 0xE58F0008, 0xE8BD8010, BANNERREFRESHHUD, BANNERANNOUNCE, 0xFFFFFFFF};
            for (u32 index = 0; index < bannerSavedCode.size(); index++)
                w32(BANNERHUDTICK + index * 4, refreshAndAnnounce[index]);
            started = true;
        }
    }
    return started;
}
void Plugins::APC::Ctx::pollObtainedBanner() {
    if (bannerActiveHud) {
        bool sameField = r32(MISSIONROOT) == bannerFieldRoot && r32(BANNERHUD) == bannerActiveHud;
        bool sameOverlay = r32(BANNERHUDTICK + 4) == 0xE59FC01C;
        bool displayed = sameOverlay && r32(BANNERHUDTICK + BANNERHUDTICKDONE) == 0;
        if (displayed) {
            for (u32 index = 0; index < bannerSavedCode.size(); index++)
                w32(BANNERHUDTICK + index * 4, bannerSavedCode[index]);
            bannerDisplayed = true;
        }
        bool bannerFinished = bannerDisplayed && !r32(bannerActiveHud + BANNERLOCKHELD) && !r32(bannerActiveHud + BANNERLOCKOWNER);
        bool attemptFailed = !bannerDisplayed && (!sameOverlay || ++bannerWaitFrames >= 8);
        if (!sameField || bannerFinished || attemptFailed || !on) {
            if (sameOverlay && !displayed)
                for (u32 index = 0; index < bannerSavedCode.size(); index++)
                    w32(BANNERHUDTICK + index * 4, bannerSavedCode[index]);
            if (r32(MISSIONROOT) == bannerFieldRoot)
                for (u32 index = 0; index < bannerSavedText.size(); index++)
                    w8(bannerTextEntry + index, bannerSavedText[index]);
            if (sameField && bannerDisplayed)
                obtainedBanners.pop_front();
            bannerActiveHud = 0;
        }
    }
    if (on && !bannerActiveHud && !obtainedBanners.empty())
        startObtainedBanner(obtainedBanners.front());
}
