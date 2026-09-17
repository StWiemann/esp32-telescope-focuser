#include "BacklashPolicy.h"

MoveSequence BacklashPolicy::compute(int32_t currentPos,
                                     int32_t targetPos,
                                     int32_t backlash,
                                     int8_t  approachDir,
                                     int32_t minPos,
                                     int32_t maxPos) {
    MoveSequence seq;
    seq.target   = targetPos;
    seq.twoPhase = false;

    if (backlash <= 0 || currentPos == targetPos) {
        return seq;  // no compensation needed
    }

    // Direction of this move: +1 = outward, -1 = inward
    int8_t moveDir = (targetPos > currentPos) ? 1 : -1;

    if (moveDir == approachDir) {
        // Moving in the preferred approach direction: go directly.
        return seq;
    }

    // Moving against the preferred approach direction: overshoot, then return.
    // Overshoot position is backlash steps FURTHER in the non-preferred direction.
    int32_t overshoot = targetPos + (-approachDir) * backlash;

    // Clamp overshoot to valid range (avoid hitting mechanical stops)
    overshoot = constrain(overshoot, minPos, maxPos);

    // If clamping eliminated the overshoot, fall back to a direct move.
    if (overshoot == targetPos) {
        return seq;
    }

    seq.twoPhase    = true;
    seq.intermediate = overshoot;
    return seq;
}
