#include "State.h"
#include "itemDatabase.h"
#include <string>
namespace Plugins::APC {
void Ctx::run() {
    pollActiveFieldGoal();
    pollParallelSora();
    pollChar();
    pollGameStart();
    pollGate();
    pollMision();
    pollCalculateScaling();
    enemyScan();
    pollRewardsBits();
    pollMissionLocking();
    pollDayLocking();
    pollReplaceChest();
    pollDetectOpenChests();
    server.pollTCP();

}

void Ctx::commandHandling(u8 opcode, u8 size, u8 *buffer) {
    logmine("opcode %02X",opcode);
    switch(opcode) {
        case 0x01: {
            if (size == 2) {
                u16 item = buffer[2] | (buffer[3] << 8);
                giveItemType(item);
            }
            break;
        }
        case 0x02: setMissionLocked(buffer[2],buffer[3]); break;
        case 0x03: currentChar = static_cast<CharIds>(buffer[2]); break;
        case 0x04: missionDone = buffer[2]; break;
        case 0x05: finalMissionUnlocked = buffer[2]; break;
        case 0x06: setMultipleMissionLocked(size,buffer); break;
        case 0x10: getInventory(); break;
        case 0x20: setCharacterMission(size,buffer); break;
        case 0x21: setCharacterRando(size,buffer); break;
        case 0x22: {
            std::u16string text;
            for (u32 offset = 0; offset + 1 < size; offset += 2)
                text.push_back(buffer[offset + 2] | (buffer[offset + 3] << 8));
            showObtainedBanner(text);
            break;
        }

    }
}

void Ctx::setCharacterRando(u8 size, u8 *buffer){
    setCharacterRandoFlag = buffer[2];
}

void Ctx::setCharacterMission(u8 size, u8 * buffer){
    for(u32 i = 0; i < size; i+=2){
        assignMissionCharacter(buffer[i+2],buffer[i+3]);
    }
    
    setCharacterRandoFlag = true;
}

void Ctx::returnToApClient(u8 opcode, u8 size, u8 * buffer) {
    std::vector<u8> packet{opcode, size};
    packet.insert(packet.end(), buffer, buffer + size);
    server.send(packet);

}
void Ctx::getInventory() {
    u32 state = r32(GAMESTATE);
    if (state) {
        u8 buffer[253];
        u8 slot = 0;
        u8 size = 1;
        buffer[0] = slot;

        for (const ItemName &item : itemDb) {
            buffer[size] = item.key & 0xFF;
            buffer[size + 1] = item.key >> 8;
            buffer[size + 2] = r8(state + ITEMCOUNT + item.key);
            size += 3;

            if (size == 253) {
                returnToApClient(0x11, size, buffer);
                slot++;
                size = 1;
                buffer[0] = slot;
            }
        }

        if (size > 1)
            returnToApClient(0x11, size, buffer);
    }
}

void Ctx::setMultipleMissionLocked(u8 amount, u8 *buffer) {
    for (u8 i = 0; i < (amount/2); i++) {
        setMissionLocked(buffer[2+(i*2)],buffer[3+(i*2)]);
    }
}


void Ctx::giveItemType(u16 item){
        bool magic = false;
        for (u8 i = 0; i < 15; i++) {
            magic |= magicKeys[i] == item;
        }
        if ((item != 1) && !magic) {
            itemGive(item);

        } 
        if( item == 1) {
            RowRelease(1);
        }
        if(magic) {
            giveMagic(item, 3);
        }
}
}
