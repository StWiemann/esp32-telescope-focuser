#pragma once
#include <Arduino.h>

/**
 * BacklashPolicy – stateless backlash compensation logic.
 *
 * Terminology
 * -----------
 * "Preferred approach direction" (approachDir):
 *   +1 = outward (motor steps increasing)
 *   -1 = inward  (motor steps decreasing)
 *
 * When backlashSteps > 0 and the move ends in the NON-preferred direction,
 * the motor first overshoots by backlashSteps in the non-preferred direction,
 * then returns to the logical target.  The logical position always reflects
 * the requested target, not the overshoot.
 *
 * Example (approachDir = +1 = outward):
 *   Move from pos=5000 to pos=3000 (inward, non-preferred)
 *   → Phase 1: motor moves to 3000 - 200 = 2800  (overshoot inward)
 *   → Phase 2: motor moves to 3000               (approach outward)
 *   Logical position reported: 3000 throughout.
 *
 * Two-phase moves are expressed as a MoveSequence struct.
 */
struct MoveSequence {
    bool    twoPhase;       // false = go directly to target
    int32_t intermediate;   // overshoot position (valid only if twoPhase)
    int32_t target;         // final logical target
};

class BacklashPolicy {
public:
    /**
     * Compute the move sequence for a requested position change.
     *
     * @param currentPos  Current logical position (motor steps).
     * @param targetPos   Requested logical target (motor steps).
     * @param backlash    Backlash compensation steps (0 = disabled).
     * @param approachDir +1 = outward approach preferred, -1 = inward.
     * @param minPos      Lower bound (usually 0).
     * @param maxPos      Upper bound.
     * @return MoveSequence describing 1- or 2-phase movement.
     */
    static MoveSequence compute(int32_t currentPos,
                                int32_t targetPos,
                                int32_t backlash,
                                int8_t  approachDir,
                                int32_t minPos,
                                int32_t maxPos);
};
