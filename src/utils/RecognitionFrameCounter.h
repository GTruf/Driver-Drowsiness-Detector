// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2024                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#ifndef RECOGNITIONCOUNTER_H
#define RECOGNITIONCOUNTER_H

#include "ImageRecognizer.h"

template <uint16_t UPPER_BOUND>
class RecognitionFrameCounter {
public:
    RecognitionFrameCounter() = default;

    bool IsFull(RecognitionType recognitionType) const {
        return _recognitionsInformation[recognitionType] == UPPER_BOUND;
    }

    bool Increase(RecognitionType recognitionType) {
        auto& recognitionFrameCount = _recognitionsInformation[recognitionType];

        if (recognitionFrameCount < UPPER_BOUND)
            recognitionFrameCount++;

        return recognitionFrameCount == UPPER_BOUND;
    }

    void Reset(RecognitionType recognitionType) {
        _recognitionsInformation[recognitionType] = 0;
    }

    void FullReset() {
        for (int currentRecognition = AttentiveEye; currentRecognition < RecognitionTypeCount; currentRecognition++) {
            Reset(static_cast<RecognitionType>(currentRecognition));
        }
    }

private:
    // [RecognitionType, Number for this RecognitionType]
    mutable std::map<RecognitionType, uint16_t> _recognitionsInformation;
};

#endif // RECOGNITIONCOUNTER_H
