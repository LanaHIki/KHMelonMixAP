#include "State.h"
#include <cmath>
namespace Plugins::APC {
u32 index = 0;
u32 currentNode = 0;
u32 currentListCount = 0;
bool once = true;
#define defenseOffset 0x2BC
#define offenseOffset 0x290
u32 listCount = 0;
u8 currentMap = 0;
u8 currentWorld = 0;
void Ctx::enemyScan() {
    u32 enemyManager = r32(0x020CBF1C);

    if (enemyManager) {
        u32 nodeBegin = r32(enemyManager + 0x08);
        u32 nodeEnd = enemyManager + 0x14;
        listCount = r32(enemyManager + 0x24);
        u8 mapCheck = r8(0x0204C3E8);
        u8 worldCheck = r8(WORLD);
        bool changed = currentMap != mapCheck || currentWorld != worldCheck;

        if (changed) {
            once = true;
            currentNode = 0;
            index = 0;
            currentMap = mapCheck;
            currentWorld = worldCheck;
        }

        if(currentNode == 0) {
          currentNode = nodeBegin;
        }

        if ((currentNode != nodeEnd) && enemyManager) {
            u32 next = r32(currentNode + 0x04);
            u32 slot = r32(currentNode + 0x0C);
            u32 obj = r32(slot);
            u32 flags = r16(obj);

            if(obj && flags){
            
            u8 enemyClass = r8(obj + 0x19C);
            bool isBarrel = enemyClass == 0x6B || enemyClass == 0x32;
            //checking if the actor is a combat actor
            if(((flags & 0x60) == 0x60) && once && !isBarrel){
                for(u16 i = 0; i < 8; i++){
                    u16 offStat = r16(obj + offenseOffset + (6*i));
                    offStat = std::max<double>(5.0,std::floor( offStat * statScalePercent / 100));
                    w16(obj + offenseOffset + (6*i), offStat);
                    u16 defStat = r16(obj + defenseOffset + (2*i));
                    defStat = std::max<double>(5.0,std::floor( defStat * statScalePercent / 100));
                    w16(obj + defenseOffset + (2*i), defStat);
                }
                u16 maxhp = r16(obj + 0x218);
                maxhp = std::max<double>(5.0,std::floor( maxhp * hpScalePercent / 100));
                w16(obj + 0x218, maxhp);
                w16(obj + 0x21A,maxhp);
            }
        }
            currentNode = next;
            index++;
    }
        if (index == listCount){
            index = 0;
            currentNode = 0;
            once = false;
        }
        if (listCount != currentListCount) {
            currentListCount = listCount;
            once = true;
            currentNode = 0;
            index = 0;
        }   
    }


}

void Ctx::pollCalculateScaling(){
    u16 currentMission = r16(MISSIONID);
    double relativeMission = currentMission - missionDone;
    if (relativeMission < 1.0) {
        relativeMission = 1.0;
    }
    if (relativeMission > 91.0) {
        relativeMission = 91.0;
    }

    double missionProgress = (relativeMission - 1.0) / 90.0;
    double hpPercent = 0.0;
    double statPercent = 0.0;


    //brackets for mission done, if more than 25% of missions are done the scaling weakens
    if (missionProgress <= 0.25) {
        hpPercent = 100.0 - 4.0 * missionProgress;
        statPercent = 100.0 - 48.0 * missionProgress;
    } else if (missionProgress <= 0.5) {
        hpPercent = 106.0 - 28.0 * missionProgress;
        statPercent = 124.0 - 144.0 * missionProgress;
    } else if (missionProgress <= 0.75) {
        hpPercent = 126.0 - 68.0 * missionProgress;
        statPercent = 96.0 - 88.0 * missionProgress;
    } else {
        hpPercent = 120.0 - 60.0 * missionProgress;
        statPercent = 90.0 - 80.0 * missionProgress;
    }

    hpScalePercent = hpPercent;
    statScalePercent = statPercent;
}

}
