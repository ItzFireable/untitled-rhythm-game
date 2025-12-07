#include <rhythm/DifficultyCalculator.h>
#include <utils/Utils.h>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <iostream>

DifficultyCalculator::DifficultyCalculator(DifficultyConfig cfg) : config(cfg) {}

void DifficultyCalculator::setConfig(const DifficultyConfig& cfg) {
    config = cfg;
}

void DifficultyCalculator::extractFeatures(ChartData& chartData, float rate) {
    processedNotes.clear();
    for (const auto& n : chartData.notes) {
        if (n.type == MINE) continue;

        int adjustedTime = static_cast<int>(n.time / rate);
        processedNotes.push_back({adjustedTime, n.column, n.type});
    }

    int keyCount = chartData.keyCount;
    if (processedNotes.empty()) return;

    std::map<int, int> timeToChordSize;
    for (const auto& n : processedNotes) {
        if (n.type == TAP || n.type == HOLD_START) {
            timeToChordSize[n.time]++;
        }
    }

    std::vector<double> lastHitTimeByColumn(keyCount, -std::numeric_limits<double>::infinity());
    CalculatedNote* prevNote = nullptr;

    for (auto& note : processedNotes) {
        if (note.type == HOLD_END) continue;

        note.colActive = timeToChordSize[note.time];
        note.isChord = note.colActive > 1;
        note.isRice = (note.type == TAP);
        note.isLN = (note.type == HOLD_START);

        note.msSincePrev = std::numeric_limits<double>::infinity();
        note.colChange = false;
        note.handSimul = false;

        double halfKey = keyCount / 2.0;
        note.isLeftCol = note.column < halfKey;
        note.isRightCol = note.column >= halfKey;

        if (prevNote) {
            note.msSincePrev = (double)note.time - prevNote->time;
            note.colChange = note.column != prevNote->column;
            
            bool prevLeft = prevNote->column < halfKey;
            bool prevRight = prevNote->column >= halfKey;
            
            if ((note.isLeftCol && prevRight) || (note.isRightCol && prevLeft)) {
                note.handSimul = true;
            }
        }

        note.msSincePrevCol = (double)note.time - lastHitTimeByColumn[note.column];
        lastHitTimeByColumn[note.column] = note.time;

        prevNote = &note;
    }
}

double DifficultyCalculator::getBpmAt(int time, const ChartData& chartData, float rate) {
    if (chartData.timingPoints.empty()) return 60.0 * rate;
    double bpm = chartData.timingPoints[0].bpm;
    for (const auto& tp : chartData.timingPoints) {
        int adjustedTime = static_cast<int>(tp.time / rate);
        if (adjustedTime <= time) bpm = tp.bpm;
        else break;
    }
    return bpm * rate;
}

double DifficultyCalculator::calcStream(const std::vector<const CalculatedNote*>& segment, double durationS) {
    if (segment.empty() || durationS <= 0) return 0;
    double score = 0;
    for (const auto* n : segment) {
        if (!n->isRice || n->colActive > 2) continue;
        double interval = n->msSincePrev;
        if (interval <= 0) continue;

        double speedFactor = 0;
        if (interval < 150) speedFactor = std::pow(80.0 / interval, 0.65);
        else if (interval < 250) speedFactor = std::pow(250.0 / interval, 0.4);

        double patternFactor = n->colChange ? 1.2 : 1.0;
        if (n->handSimul) patternFactor *= 1.3;

        score += speedFactor * patternFactor;
    }
    return std::pow(std::pow(score / durationS, 0.85) * 0.7, config.RICE_SKILL_EXPONENT);
}

double DifficultyCalculator::calcJumpstream(const std::vector<const CalculatedNote*>& segment, double durationS) {
    if (segment.empty() || durationS <= 0) return 0;
    double score = 0;
    for (const auto* n : segment) {
        double interval = n->msSincePrev;
        if (interval <= 0) continue;
        
        if (n->isRice && n->colActive >= 2 && n->colActive <= 3) {
            double speedFactor = 1.0;
            if (interval < 300) speedFactor = std::pow(300.0 / interval, 0.4);
            score += std::pow(n->colActive, 2) * speedFactor;
        }
    }
    return std::pow(std::pow(score / durationS, 0.7) * 0.7, config.RICE_SKILL_EXPONENT);
}

double DifficultyCalculator::calcHandstream(const std::vector<const CalculatedNote*>& segment, double durationS) {
    if (segment.empty() || durationS <= 0) return 0;
    double score = 0;
    for (const auto* n : segment) {
        double interval = n->msSincePrev;
        if (interval <= 0) continue;

        if (n->isRice && (n->colActive >= 3 || n->handSimul)) {
            double speedFactor = 1.0;
            if (interval < 250) speedFactor = std::pow(250.0 / interval, 0.5);

            double jackPenalty = 1.0;
            double colInt = n->msSincePrevCol;
            if (colInt > 0 && colInt < config.JACK_MIN_INTERVAL_MS) {
                jackPenalty = 1.0 / std::pow(config.JACK_MIN_INTERVAL_MS / colInt, 1.2);
            }
            
            if (n->isRice && n->colActive <= 2 && interval < 150) {
                 score += std::pow(150.0 / interval, 0.4);
            }

            score += std::pow(n->colActive, 1.5) * speedFactor * (n->handSimul ? 1.5 : 1.0) * jackPenalty;
        }
    }
    return std::pow(std::pow(score / durationS, 0.8) * 0.75, config.RICE_SKILL_EXPONENT);
}

double DifficultyCalculator::calcJack(const std::vector<const CalculatedNote*>& segment, int keyCount, double durationS) {
    if (segment.empty() || durationS <= 0) return 0;
    double score = 0;
    for (const auto* n : segment) {
        if (!n->isRice) continue;
        double colInt = n->msSincePrevCol;
        if (colInt <= 0) continue;

        if (colInt < config.JACK_MIN_INTERVAL_MS) {
            score += std::pow(config.JACK_MIN_INTERVAL_MS / colInt, 1.2) * (1.0 + (n->colActive * 0.5));
        }
    }
    return std::pow(std::pow(score / durationS / keyCount, 0.7) * 0.9, config.RICE_SKILL_EXPONENT);
}

double DifficultyCalculator::calcChordjack(const std::vector<const CalculatedNote*>& segment, int keyCount, double durationS) {
    if (segment.empty() || durationS <= 0) return 0;
    double score = 0;
    for (const auto* n : segment) {
        if (!n->isRice || n->colActive <= 1) continue;
        double colInt = n->msSincePrevCol;
        if (colInt <= 0) continue;

        if (colInt < config.JACK_MIN_INTERVAL_MS) {
            double speedFactor = std::pow(config.JACK_MIN_INTERVAL_MS / colInt, 1.1);
            double chordFactor = std::pow(n->colActive, 1.5);
            score += speedFactor * chordFactor;
        }
    }
    return std::pow(std::pow(score / durationS / keyCount, 0.6), config.RICE_SKILL_EXPONENT);
}

double DifficultyCalculator::calcTechnical(const std::vector<const CalculatedNote*>& segment) {
    if (segment.size() <= 1) return 0;
    std::vector<double> intervals;
    for (size_t i = 1; i < segment.size(); ++i) {
        double d = (double)segment[i]->time - segment[i-1]->time;
        if (d > 0) intervals.push_back(d);
    }
    
    double sd = Utils::calculateStandardDeviation(intervals);
    double avg = 0;
    if (!intervals.empty()) avg = std::accumulate(intervals.begin(), intervals.end(), 0.0) / intervals.size();

    if (avg == 0 || sd == 0) return 0;

    double varianceFactor = (sd / avg) * (250.0 / avg);
    
    double patternVariance = 0;
    for (size_t i = 1; i < segment.size(); ++i) {
        int change = std::abs(segment[i]->colActive - segment[i-1]->colActive);
        patternVariance += std::pow(change, 2);
    }
    double patternFactor = patternVariance / segment.size();

    double raw = varianceFactor + (patternFactor * 3.5);
    return std::pow(raw * 0.8, config.RICE_SKILL_EXPONENT);
}

double DifficultyCalculator::calcLnDensity(const std::vector<const CalculatedNote*>& starts, double durationS) {
    if (starts.empty() || durationS <= 0) return 0;
    double lnps = starts.size() / durationS;
    return std::pow(std::pow(lnps, 1.1) * 1.2, config.LN_SKILL_EXPONENT);
}

double DifficultyCalculator::calcLnSpeed(const std::vector<const CalculatedNote*>& starts) {
    double score = 0;
    for (size_t i = 1; i < starts.size(); ++i) {
        double interval = (double)starts[i]->time - starts[i-1]->time;
        if (interval > 0 && interval < 100) {
            score += (100.0 / interval) * 0.5;
        }
    }
    return std::pow(std::pow(score, 0.6), config.LN_SKILL_EXPONENT);
}

double DifficultyCalculator::calcLnShields(const std::vector<const CalculatedNote*>& rice, const std::vector<const CalculatedNote*>& lnEvents) {
    if (lnEvents.size() < 2) return 0;
    std::map<int, const CalculatedNote*> activeLNs;
    double score = 0;

    for (const auto* n : lnEvents) {
        if (n->type == HOLD_START) {
            activeLNs[n->column] = n;
        } else if (n->type == HOLD_END && activeLNs.count(n->column)) {
            const CalculatedNote* start = activeLNs[n->column];
            for (const auto* r : rice) {
                if (r->time > start->time && r->time < n->time) {
                    int dist = std::abs(r->column - n->column);
                    double distFactor = 1.0 + (1.0 / (dist + 0.1));
                    if (r->column != n->column) {
                        score += 1.2 * distFactor;
                    }
                }
            }
            activeLNs.erase(n->column);
        }
    }
    return std::pow(std::pow(score / 50.0, 1.2), config.LN_SKILL_EXPONENT);
}

double DifficultyCalculator::calcLnComplexity(const std::vector<const CalculatedNote*>& lnEvents, int keyCount) {
    double score = 0;
    for (size_t i = 1; i < lnEvents.size(); ++i) {
        double interval = (double)lnEvents[i]->time - lnEvents[i-1]->time;
        if (interval > 0 && interval < 300) {
            if (lnEvents[i]->column != lnEvents[i-1]->column) {
                score += (300.0 / interval) * 0.5;
            }
            if (lnEvents[i-1]->time == lnEvents[i]->time && lnEvents[i-1]->type != lnEvents[i]->type) {
                score += 5.0;
            }
        }
    }
    return std::pow(std::pow(score, 0.7) / keyCount, config.LN_SKILL_EXPONENT);
}

double DifficultyCalculator::calcStamina(const std::vector<SkillResult>& segments) {
    double windowLen = config.STAMINA_WINDOW_S * 1000.0;
    int segsPerWindow = std::floor(windowLen / config.WINDOW_OVERLAP_MS);
    
    if (segments.size() < (size_t)segsPerWindow) return 0;

    double maxStamina = 0;
    double wS = 1.1, wJS = 1.0, wHS = 1.1, wJ = 1.2, wCJ = 0.7, wT = 0.9;

    for (size_t i = 0; i <= segments.size() - segsPerWindow; ++i) {
        double windowSum = 0;
        for (int j = 0; j < segsPerWindow; ++j) {
            const auto& s = segments[i + j];
            double segScore = (s.stream * wS) + (s.jumpstream * wJS) + (s.handstream * wHS) 
                            + (s.jack * wJ) + (s.chordjack * wCJ) + (s.technical * wT);
            windowSum += segScore;
        }
        double windowAvg = windowSum / segsPerWindow;
        maxStamina = std::max(maxStamina, windowAvg);
    }

    return std::pow(std::pow(maxStamina, 0.75), config.RICE_SKILL_EXPONENT);
}


FinalResult DifficultyCalculator::calculate(ChartData& chartData, float rate) {
    if (rate <= 0.0f) rate = 1.0f;
    
    extractFeatures(chartData, rate);

    int keyCount = chartData.keyCount;
    if (processedNotes.empty()) {
        return FinalResult{"", "", 1.0f, keyCount, 0.0, SkillResult{}, 0.0, 0.0, 0.0, 0.0};
    }

    std::vector<const CalculatedNote*> ricePtrs;
    std::vector<const CalculatedNote*> lnEventPtrs;
    std::vector<const CalculatedNote*> lnStartPtrs;

    for (const auto& n : processedNotes) {
        if (n.isRice) ricePtrs.push_back(&n);
        if (n.type == HOLD_START || n.type == HOLD_END) lnEventPtrs.push_back(&n);
        if (n.type == HOLD_START) lnStartPtrs.push_back(&n);
    }

    int startTime = processedNotes.front().time;
    int endTime = processedNotes.back().time;

    std::vector<SkillResult> segmentResults;
    SkillResult maxSkills;

    for (double ct = startTime; ct <= endTime; ct += config.WINDOW_OVERLAP_MS) {
        double wEnd = ct + config.WINDOW_MS;
        double durMs = std::min((double)wEnd, (double)endTime) - ct;
        if (durMs < 1000) continue;
        double durSec = durMs / 1000.0;

        auto getSubset = [&](const std::vector<const CalculatedNote*>& src) {
            std::vector<const CalculatedNote*> sub;
            for (const auto* n : src) {
                if (n->time >= ct && n->time < wEnd) sub.push_back(n);
            }
            return sub;
        };

        auto segRice = getSubset(ricePtrs);
        auto segLns = getSubset(lnEventPtrs);
        auto segLnStarts = getSubset(lnStartPtrs);

        double chartBpm = getBpmAt(ct, chartData, rate);
        double nps = segRice.size() / durSec;
        double densityBpm = nps * 60.0;
        double finalBpm = std::min(chartBpm, densityBpm);
        finalBpm = std::min(finalBpm, config.MAX_BPM_RATIO * config.BPM_BASE);
        double bpmScale = std::pow(finalBpm / config.BPM_BASE, config.BPM_SCALE_EXPONENT);

        SkillResult sr;
        sr.stream = calcStream(segRice, durSec) * bpmScale;
        sr.jumpstream = calcJumpstream(segRice, durSec) * bpmScale;
        sr.handstream = calcHandstream(segRice, durSec) * bpmScale;
        sr.jack = calcJack(segRice, keyCount, durSec) * bpmScale;
        sr.chordjack = calcChordjack(segRice, keyCount, durSec) * bpmScale;
        sr.technical = calcTechnical(segRice) * bpmScale;
        
        sr.density = calcLnDensity(segLnStarts, durSec) * bpmScale;
        sr.speed = calcLnSpeed(segLnStarts) * bpmScale;
        sr.shields = calcLnShields(segRice, segLns) * bpmScale;
        sr.complexity = calcLnComplexity(segLns, keyCount) * bpmScale;

        segmentResults.push_back(sr);

        maxSkills.stream = std::max(maxSkills.stream, sr.stream);
        maxSkills.jumpstream = std::max(maxSkills.jumpstream, sr.jumpstream);
        maxSkills.handstream = std::max(maxSkills.handstream, sr.handstream);
        maxSkills.jack = std::max(maxSkills.jack, sr.jack);
        maxSkills.chordjack = std::max(maxSkills.chordjack, sr.chordjack);
        maxSkills.technical = std::max(maxSkills.technical, sr.technical);
        maxSkills.density = std::max(maxSkills.density, sr.density);
        maxSkills.speed = std::max(maxSkills.speed, sr.speed);
        maxSkills.shields = std::max(maxSkills.shields, sr.shields);
        maxSkills.complexity = std::max(maxSkills.complexity, sr.complexity);
    }

    maxSkills.stamina = calcStamina(segmentResults);

    double riceFinal = Utils::pNorm(
        {maxSkills.stream, maxSkills.jumpstream, maxSkills.handstream, maxSkills.jack, maxSkills.chordjack, maxSkills.technical, maxSkills.stamina},
        {config.W_RICE_STREAM, config.W_RICE_JUMPSTREAM, config.W_RICE_HANDSTREAM, config.W_RICE_JACK, config.W_RICE_CHORDJACK, config.W_RICE_TECHNICAL, config.W_RICE_STAMINA},
        config.P_NORM_EXPONENT
    );

    double lnFinal = Utils::pNorm(
        {maxSkills.density, maxSkills.speed, maxSkills.shields, maxSkills.complexity},
        {config.W_LN_DENSITY, config.W_LN_SPEED, config.W_LN_SHIELDS, config.W_LN_COMPLEXITY},
        config.P_NORM_EXPONENT
    );

    double scaledRiceFinal = std::pow(riceFinal, config.RICE_SKILL_EXPONENT);
    double scaledLnFinal = std::pow(lnFinal, config.LN_SKILL_EXPONENT);

    double finalRiceWeight = config.W_RICE_FINAL;
    double finalLnWeight = config.W_LN_FINAL;

    const double EPSILON = 1e-6;
    const double DOMINANCE_THRESHOLD = 0.65;

    double sum = scaledRiceFinal + scaledLnFinal;

    if (sum > EPSILON) {
        if (scaledRiceFinal / sum > DOMINANCE_THRESHOLD) {
            finalRiceWeight *= 1.1;
            finalLnWeight *= 0.8;
        } 

        else if (scaledLnFinal / sum > DOMINANCE_THRESHOLD) {
            finalRiceWeight *= 0.8;
            finalLnWeight *= 1.1;
        }
    }

    double finalTotal = Utils::pNorm(
        {scaledRiceFinal, scaledLnFinal}, 
        {finalRiceWeight, finalLnWeight},
        config.P_NORM_EXPONENT
    );

    double chartMinBPM = std::numeric_limits<double>::max();
    double chartMaxBPM = std::numeric_limits<double>::min();

    if (!chartData.timingPoints.empty()) {
        for (const auto& tp : chartData.timingPoints) {
            double adjustedBpm = tp.bpm * rate;
            if (adjustedBpm > chartMaxBPM) {
                chartMaxBPM = adjustedBpm;
            }
            if (adjustedBpm < chartMinBPM) {
                chartMinBPM = adjustedBpm;
            }
        }
    } else {
        chartMinBPM = 0.0; 
        chartMaxBPM = 0.0;
    }

    FinalResult fr;
    fr.playbackRate = rate;
    fr.title = chartData.metadata["title"];
    fr.difficulty = chartData.metadata["difficulty"];
    fr.keyCount = keyCount;
    fr.rawDiff = finalTotal;
    fr.skills = maxSkills;
    fr.riceTotal = riceFinal;
    fr.lnTotal = lnFinal;

    fr.minBPM = chartMinBPM;
    fr.maxBPM = chartMaxBPM;

    return fr;
}