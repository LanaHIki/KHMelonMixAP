#include "State.h"
#include "../../Platform.h"
namespace Plugins::APC {
bool withdrawn = false;
void Ctx::pollMision() {
    //Setting the mission to skip giving fixed and weighted table rewards. specifically the first bit of nOption64
    u8 flags = r8(RESULTREC + RESULTFLAGS);
    bool set = (flags & 1) == 0;
    if (set) {
        flags = flags | 1;
        w8(RESULTREC + RESULTFLAGS, flags);
        if (!withdrawn) {
            u16 missionID = r16(MISSIONID);
            bool mine = missionID == 37 || missionID == 91 || missionID == 88 || missionID == 74;
            bool cleared = missionResult == ACHIEVED || missionResult == OVERACHIEVED;
            u8 buffer[2];
            buffer[0] = static_cast<u8>(missionID);
            buffer[1] = static_cast<u8>(missionResult);
            returnToApClient(0x07, 0x02, buffer);
        }

        missionResult = NOT_ACHIEVED;
        withdrawn = false;
}
}

void Ctx::pollActiveFieldGoal(){
    u32 fieldContext = r32(FIELDCONTEXT);

    if (fieldContext != 0) {
        u32 total = 0;
        u32 done = 0;
        u32 goal = 0;
        total = r32(fieldContext + FIELDMISSIONTOTALOFFSET);
        goal = r32(fieldContext + FIELDMISSIONGOALOFFSET);
        done = r32(fieldContext + FIELDMISSIONDONEOFFSET);
        if (total !=0){
            bool goalAchieved = done >= goal;
            bool totalAchieved = done == total;
            if (goalAchieved) missionResult = totalAchieved ? OVERACHIEVED : ACHIEVED;

        } else missionResult = NOT_ACHIEVED;
     
    }
    u32 gameState = r32(GAMESTATE);
    bool withdrew = false;
    if (gameState) {
        u8 flags = r8(gameState + 0x423);
        withdrew = (flags & 1) != 0;
        if(withdrew) {
            withdrawn = true;
            missionResult = NOT_ACHIEVED;
        }
        }
}
}
