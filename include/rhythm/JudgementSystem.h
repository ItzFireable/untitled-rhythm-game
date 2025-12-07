#ifndef JUDGEMENT_COUNTER_H
#define JUDGEMENT_COUNTER_H

#include <string>
#include <map>
#include <vector>

#include <SDL3/SDL.h>

enum class Judgement {
    First = 0,

    Marvelous = First,
    Perfect,
    Great,
    Good,
    Bad,
    Miss,

    Last = Miss
};

struct JudgementData {
    Judgement judgement;
    SDL_Color color;
    int windowMs;
};

static const JudgementData judgementData[] = {
    { Judgement::Marvelous, {255, 255, 255, 255}, 22 },
    { Judgement::Perfect, {255, 237, 37, 255}, 45 },
    { Judgement::Great, {140, 232, 130, 255}, 90 },
    { Judgement::Good, {106, 168, 255, 255}, 135 },
    { Judgement::Bad, {183, 130, 232, 255}, 180 },
    { Judgement::Miss, {255, 0, 0, 255}, 181 } 
};

struct JudgementResult {
    Judgement judgement;
    float offsetMs;
};

class JudgementSystem
{
public:
    JudgementSystem();
    ~JudgementSystem();
    
    void addJudgement(Judgement j, float offsetMs);
    void reset();

    int getTotalNotes();
    int getJudgementCount(Judgement j);
    float getAccuracy();
    float getMaxMissWindowMs();

    JudgementResult getJudgementForTimingOffset(float offsetMs, bool isRelease = false);

    static std::vector<Judgement> getAllJudgements() {
        std::vector<Judgement> judgements;
        int first = static_cast<int>(Judgement::First);
        int last = static_cast<int>(Judgement::Last);
        for (int i = first; i <= last; ++i) {
            judgements.push_back(static_cast<Judgement>(i));
        }
        return judgements;
    }

    static std::string judgementToString(Judgement j)
    {
        switch (j)
        {
        case Judgement::Marvelous:
            return "Marvelous";
        case Judgement::Perfect:
            return "Perfect";
        case Judgement::Great:
            return "Great";
        case Judgement::Good:
            return "Good";
        case Judgement::Bad:
            return "Bad";
        case Judgement::Miss:
            return "Miss";
        default:
            return "";
        }
    }

    static SDL_Color judgementToColor(Judgement j)
    {
        for (const auto& data : judgementData)
        {
            if (data.judgement == j)
            {
                return data.color;
            }
        }
        return {255, 255, 255, 255};
    }

    std::vector<JudgementResult> getResults() const {
        return results_;
    }

    std::map<Judgement, int> getCounts() const {
        return counts_;
    }
private:
    std::map<Judgement, int> counts_;
    std::vector<JudgementResult> results_;
};

#endif