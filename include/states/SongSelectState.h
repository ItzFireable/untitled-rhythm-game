#ifndef SONG_SELECT_STATE_H
#define SONG_SELECT_STATE_H

#include <BaseState.h>
#include <rhythm/ChartManager.h>
#include <rhythm/Conductor.h>
#include <rhythm/DifficultyCalculator.h>
#include <objects/TextObject.h>
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <map>

class SongSelectState : public BaseState
{
public:
    void init(AppContext* appContext, void* payload) override;
    void handleEvent(const SDL_Event& e) override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

    std::string getStateName() const override {
        return "SongSelectState";
    }
private:
    std::unique_ptr<Conductor> conductor_;

    std::vector<ChartData> charts_;
    std::vector<TextObject*> chartTitles_;
    int selectedIndex_ = 0;
    float selectedRate = 1.0f;
    
    TextObject* diffTextObject_ = nullptr;
    TextObject* chartInfoText_ = nullptr;

    DifficultyCalculator calculator;
    std::map<std::string, FinalResult> difficultyCache_;
    
    float listCenterX_ = 0.0f;
    float listCenterY_ = 0.0f;
    float listYOffset_ = 0.0f;
    float targetYOffset_ = 0.0f;
    int lineSkip_ = 32;

    std::string getAudioPath(const ChartData& chartData);
    std::string getChartTitle(ChartData chartData);
    std::string getChartInfo(const ChartData& chartData);

    void updateChartPositions();
    void updateDifficultyDisplay(ChartData chartData);
    void playChartPreview(const ChartData& chartData);
    void updateSelectedChartInfo(bool playPreview = true);
};

#endif