#ifndef JUDGEMENT_COUNTER_H
#define JUDGEMENT_COUNTER_H

#include <string>
#include <map>
#include <vector>

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
    int windowMs;
};

static const JudgementData judgementData[] = {
    { Judgement::Marvelous, 22 },
    { Judgement::Perfect, 45 },
    { Judgement::Great, 90 },
    { Judgement::Good, 135 },
    { Judgement::Bad, 180 },
    { Judgement::Miss, 181 } 
};

class JudgementSystem
{
public:
    JudgementSystem();
    ~JudgementSystem();
    
    void addJudgement(Judgement j);
    void reset();

    int getTotalNotes();
    int getJudgementCount(Judgement j);
    float getAccuracy();
    float getMaxMissWindowMs();

    Judgement getJudgementForTimingOffset(float offsetMs, bool isRelease = false);

    std::vector<Judgement> getAllJudgements() {
        std::vector<Judgement> judgements;
        int first = static_cast<int>(Judgement::First);
        int last = static_cast<int>(Judgement::Last);
        for (int i = first; i <= last; ++i) {
            judgements.push_back(static_cast<Judgement>(i));
        }
        return judgements;
    }

    inline std::string judgementToString(Judgement j)
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

private:
    std::map<Judgement, int> counts_;
};

#endif