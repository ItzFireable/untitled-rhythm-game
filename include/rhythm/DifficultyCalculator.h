#ifndef DIFFICULTY_CALCULATOR_H
#define DIFFICULTY_CALCULATOR_H

#include "utils/Utils.h"        
#include "utils/rhythm/ChartUtils.h" 

struct CalculatedNote {
    int time;
    int column;
    NoteType type;

    int colActive = 0;
    bool isChord = false;
    bool isRice = false;
    bool isLN = false;
    
    double msSincePrev = 0.0; 
    double msSincePrevCol = 0.0;    
    bool colChange = false;
    
    bool isLeftCol = false;
    bool isRightCol = false;
    bool handSimul = false;
};

struct DifficultyConfig {
    double W_RICE_FINAL = 0.85;
    double W_LN_FINAL = 0.725;

    double P_NORM_EXPONENT = 8.0;

    double RICE_SKILL_EXPONENT = 1.0;
    double LN_SKILL_EXPONENT = 0.9;

    double W_RICE_STREAM = 0.85;
    double W_RICE_JUMPSTREAM = 0.8;
    double W_RICE_HANDSTREAM = 0.65;
    double W_RICE_STAMINA = 1.25;
    double W_RICE_JACK = 1.0;
    double W_RICE_CHORDJACK = 1.1;
    double W_RICE_TECHNICAL = 1.2;

    double W_LN_DENSITY = 1.0;
    double W_LN_SPEED = 1.1;
    double W_LN_SHIELDS = 1.3;
    double W_LN_COMPLEXITY = 1.2;

    double BPM_BASE = 150.0;
    double BPM_SCALE_EXPONENT = 0.6;
    double MAX_BPM_RATIO = 1.4;

    double WINDOW_MS = 3000.0;
    double WINDOW_OVERLAP_MS = 700.0;
    double DIFF_BASE_EXPONENT = 1.6;
    double JACK_MIN_INTERVAL_MS = 500.0;
    double STAMINA_WINDOW_S = 22.0;
};

struct SkillResult {
    double stream = 0;
    double jumpstream = 0;
    double handstream = 0;
    double jack = 0;
    double chordjack = 0;
    double technical = 0;
    double stamina = 0;
    
    double density = 0;
    double speed = 0;
    double shields = 0;
    double complexity = 0;
};

struct FinalResult {
    std::string title;
    std::string difficulty;
    float playbackRate;
    int keyCount;
    double rawDiff;
    SkillResult skills;
    double riceTotal;
    double lnTotal;

    double minBPM;
    double maxBPM;
};

class DifficultyCalculator {
public:
    DifficultyCalculator(DifficultyConfig cfg = DifficultyConfig());
    
    FinalResult calculate(ChartData& chartData, float rate = 1.0);

    void setConfig(const DifficultyConfig& cfg);

private:
    DifficultyConfig config;
    std::vector<CalculatedNote> processedNotes;

    void extractFeatures(ChartData& chartData, float rate);
    double getBpmAt(int time, const ChartData& chartData, float rate);
    
    double calcStream(const std::vector<const CalculatedNote*>& segment, double durationS);
    double calcJumpstream(const std::vector<const CalculatedNote*>& segment, double durationS);
    double calcHandstream(const std::vector<const CalculatedNote*>& segment, double durationS);
    double calcJack(const std::vector<const CalculatedNote*>& segment, int keyCount, double durationS);
    double calcChordjack(const std::vector<const CalculatedNote*>& segment, int keyCount, double durationS);
    double calcTechnical(const std::vector<const CalculatedNote*>& segment);
    
    double calcLnDensity(const std::vector<const CalculatedNote*>& starts, double durationS);
    double calcLnSpeed(const std::vector<const CalculatedNote*>& starts);
    double calcLnShields(const std::vector<const CalculatedNote*>& rice, const std::vector<const CalculatedNote*>& lnEvents);
    double calcLnComplexity(const std::vector<const CalculatedNote*>& lnEvents, int keyCount);
    
    double calcStamina(const std::vector<SkillResult>& segments);
};

#endif