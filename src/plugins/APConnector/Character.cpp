#include "State.h"
#include "../../Platform.h"
#include "missionDatabase.h"
namespace Plugins::APC {
void Ctx::pollParallelSora() {
    u32 scene = r32(SCENEID);
    u32 missionRoot = r32(MISSIONROOT);
    if ((scene == 2 || scene == 19) && missionRoot >= MAINRAMSTART && missionRoot <= LASTSCENESTART) {
        u32 archiveStart = missionRoot + SCRIPTBUFFER;
        u32 archiveBytes = r32(missionRoot + SCRIPTSTREAM);
        if (archiveBytes >= 16 && archiveBytes <= SCRIPTCAPACITY && r32(archiveStart) == 0x504B4143) {
            u32 archiveEnd = archiveStart + archiveBytes;
            u32 codeTable = r32(archiveStart + SCRIPTCODETABLE);
            if (codeTable >= archiveStart && codeTable <= archiveEnd - SCRIPTWORD) {
                u32 scriptCount = r32(codeTable);
                for (u32 index = 0; index < scriptCount && scriptCount <= (archiveEnd - codeTable - SCRIPTWORD) / SCRIPTWORD; index++) {
                    u32 scriptStart = r32(codeTable + SCRIPTWORD + index * SCRIPTWORD);
                    if (scriptStart >= archiveStart && scriptStart <= archiveEnd - SCRIPTWORD) {
                        u32 codeBytes = r32(scriptStart);
                        if (codeBytes >= SCRIPTWORD && codeBytes <= archiveEnd - scriptStart) {
                            u32 codeEnd = scriptStart + codeBytes;
                            u32 command = scriptStart + SCRIPTWORD;
                            while (command + SCRIPTWORD <= codeEnd) {
                                u32 commandBytes = (r16(command + SCRIPTLENGTH) & 0x7FF) * SCRIPTWORD;
                                if (commandBytes >= SCRIPTWORD && commandBytes <= codeEnd - command) {
                                    u32 header = r32(command);
                                    if (header == 0x00030002 && r32(command + SCRIPTWORD) == 1) {
                                        u32 phase = r32(command + SCRIPTINTEGEROPERAND);
                                        if (phase == 5) w8(command + SCRIPTINTEGEROPERAND, 1);
                                        if (phase == 6) w8(command + SCRIPTINTEGEROPERAND, 2);
                                    }
                                    bool enablesSora = header == 0x00050000 && r32(command + SCRIPTWORD) == 4 && r32(command + SCRIPTINTEGEROPERAND) == 0x000120EE
                                        && r32(command + SCRIPTASSIGNSOURCE) == 1 && r32(command + SCRIPTASSIGNVALUE) == 1;
                                    if (enablesSora) w8(command + SCRIPTASSIGNVALUE, 0);
                                    if (header == 0x00034802) w8(command + SCRIPTBYTECODEOPCODE, 0x2B);
                                    command += commandBytes;
                                } else command = codeEnd;
                            }
                        }
                    }
                }
            }
        }
    }
}

void Ctx::pollChar() {
        const u32 scene = r32(SCENEID);
        if (scene == 2 || scene == 19) {
            const u32 liveSlot0Kind = r32(SESSIONTAB + 4);
            const u8 missionRecordKind = r8(MISSIONREC + MEMBERKIND);
            if (r32(SESSIONTAB)) {
                u16 missionId = r16(MISSIONID);
                u8 selectedCharacter = static_cast<u8>(currentChar);
                if (setCharacterRandoFlag)
                    for (HoloMissions &mission : missionDb)
                        if (mission.missionID == missionId) selectedCharacter = mission.setChar;
                if (liveSlot0Kind != selectedCharacter) w32(SESSIONKIND, selectedCharacter);
                if (missionRecordKind != selectedCharacter) w8(MISSIONKIND, selectedCharacter);
            }
        }
}
void Ctx::assignMissionCharacter(u16 missionID, u8 character){
    for (HoloMissions& missiondb : missionDb){
        if(missiondb.missionID == missionID) missiondb.setChar = character;
    }
}
}