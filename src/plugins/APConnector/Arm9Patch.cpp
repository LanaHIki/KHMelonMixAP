#include "State.h"

namespace Plugins::APC {
u32 encodeArm9FunctionCall(u32 callAddress, u32 targetAddress, InstructionSet targetInstructionSet) {
    // When the ARM9 runs this instruction, it saves the return address and jumps to the game function.
    // BL stays in ARM mode. BLX switches to Thumb. The game function returns to the next instruction in the patch.
    constexpr u32 callArm = 0xEB000000;
    constexpr u32 callThumb = 0xFA000000;
    // An ARM branch measures its offset from eight bytes past the instruction.
    u32 byteOffset = targetAddress - (callAddress + 8);
    u32 wordOffset = (byteOffset >> 2) & 0xFFFFFF;
    u32 callInstruction = callArm | wordOffset;
    if (targetInstructionSet == InstructionSet::Thumb) {
        // A Thumb target can also be two bytes past a word boundary.
        u32 thumbHalfwordBit = (byteOffset & 2) << 23;
        callInstruction = callThumb | thumbHalfwordBit | wordOffset;
    }
    return callInstruction;
}

void Ctx::installArm9Patch(u32 patchAddress, const u32* replacementWords, u32* savedOriginalWords, u32 wordCount) {
    // Save every original word before overwriting any of them.
    for (u32 wordIndex = 0; wordIndex < wordCount; wordIndex++)
        savedOriginalWords[wordIndex] = r32(patchAddress + wordIndex * sizeof(u32));

    // Write to emulated DS RAM. The ARM9 runs the new code when the game calls this address.
    for (u32 wordIndex = 0; wordIndex < wordCount; wordIndex++)
        w32(patchAddress + wordIndex * sizeof(u32), replacementWords[wordIndex]);
}

void Ctx::restoreArm9Patch(u32 patchAddress, const u32* savedOriginalWords, u32 wordCount) {
    // Write the saved words back to the same addresses in emulated DS RAM.
    for (u32 wordIndex = 0; wordIndex < wordCount; wordIndex++)
        w32(patchAddress + wordIndex * sizeof(u32), savedOriginalWords[wordIndex]);
}
}
