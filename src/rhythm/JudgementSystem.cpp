#include "rhythm/JudgementSystem.h"
#include <utils/Logger.h>
#include <algorithm>
#include <cmath>

JudgementSystem::~JudgementSystem() {}
JudgementSystem::JudgementSystem() {
    reset();
}

void JudgementSystem::reset() {
    counts_.clear();

    for (int i = static_cast<int>(Judgement::First); i <= static_cast<int>(Judgement::Last); ++i) {
        counts_[static_cast<Judgement>(i)] = 0;
    }
}

void JudgementSystem::addJudgement(Judgement j) {
    counts_[j]++;
}

int JudgementSystem::getJudgementCount(Judgement j) {
    auto it = counts_.find(j);
    return (it != counts_.end()) ? it->second : 0;
}

int JudgementSystem::getTotalNotes() {
    int total = 0;
    for (const auto& pair : counts_) {
        total += pair.second;
    }
    return total;
}

JudgementResult JudgementSystem::getJudgementForTimingOffset(float offsetMs, bool isRelease) {
    float absOffset = std::fabs(offsetMs);

    for (const auto& entry : judgementData) {
        float windowMs = entry.windowMs;
        if (isRelease)
            windowMs *= 1.5f;

        if (absOffset <= windowMs) {
            return { entry.judgement, absOffset };
        }
    }

    return { Judgement::Miss, absOffset };
}

float JudgementSystem::getMaxMissWindowMs() {
    return judgementData[static_cast<int>(Judgement::Last)].windowMs;
}

float JudgementSystem::getAccuracy() {
    int total = getTotalNotes();
    if (total == 0) return 100.0f;

    const float MARVELOUS_SCORE = 1.0f;
    const float PERFECT_SCORE = 1.0f;
    const float GREAT_SCORE = 0.8f;
    const float GOOD_SCORE = 0.5f;
    const float BAD_SCORE = 0.1f;
    const float MISS_SCORE = 0.0f;

    float scorePoints = 
        (counts_.at(Judgement::Marvelous) * MARVELOUS_SCORE) +
        (counts_.at(Judgement::Perfect) * PERFECT_SCORE) +
        (counts_.at(Judgement::Great) * GREAT_SCORE) +
        (counts_.at(Judgement::Good) * GOOD_SCORE) +
        (counts_.at(Judgement::Bad) * BAD_SCORE) +
        (counts_.at(Judgement::Miss) * MISS_SCORE);

    float maxScore = (float)total * MARVELOUS_SCORE;
    return (scorePoints / maxScore) * 100.0f;
}