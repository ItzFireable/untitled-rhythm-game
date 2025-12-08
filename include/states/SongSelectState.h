#ifndef SONG_SELECT_STATE_H
#define SONG_SELECT_STATE_H

#include <BaseState.h>
#include <utils/rhythm/ChartUtils.h>
#include <rhythm/Conductor.h>
#include <rhythm/DifficultyCalculator.h>
#include <objects/TextObject.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <string>
#include <map>
#include <memory>

struct SongEntry {
    std::string title;
    std::string artist;
    std::string folderPath;
    std::map<std::string, ChartData> difficulties;
};

struct SongPack {
    std::string name;
    std::vector<SongEntry> songs;
    bool isExpanded = false;
};

struct FlatSongEntry {
    bool isPackHeader;
    int packIndex;
    int songIndex;
    std::string displayText;
};

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
    Conductor* conductor_;

    std::vector<SongPack> songPacks_;
    std::vector<FlatSongEntry> flatSongList_;

    SDL_Texture* currentBackgroundTexture_ = nullptr;
    SDL_Texture* nextBackgroundTexture_ = nullptr;
    float crossfadeTimer_ = 0.0f;
    bool isFadingOut_ = false;
    std::string lastLoadedChartIdentifier_ = "";
    
    int selectedIndex_ = 0; 
    int lastSelectedIndex_ = -1; 
    
    std::string currentSelectedDifficultyName_ = "";
    float selectedRate = 1.0f;

    Uint64 lastInputTime_ = 0; 
    const Uint32 PREVIEW_DEBOUNCE_MS = 700;
    
    TextObject* diffTextObject_ = nullptr;
    TextObject* chartInfoText_ = nullptr;
    std::vector<TextObject*> chartTitles_;

    DifficultyCalculator calculator;
    std::map<std::string, FinalResult> difficultyCache_;
    
    float listCenterX_ = 0.0f;
    float listCenterY_ = 0.0f;
    float listYOffset_ = 0.0f;
    float targetYOffset_ = 0.0f;
    int lineSkip_ = 32;

    std::string getChartTitle(const ChartData& chartData);
    std::string getChartInfo(const ChartData& chartData);
    ChartData& getCurrentSelectedChart();
    const ChartData& getCurrentSelectedChart() const;
    
    SongEntry loadSongEntry(const std::string& songDirectory, const std::vector<std::string>& chartFiles);
    void loadAndCrossfadeBackground(const ChartData& chartData);
    
    void buildFlatSongList();
    void updateChartPositions();
    void updateDifficultyDisplay(ChartData& chartData);
    void playChartPreview(const ChartData& chartData);
    void updateSelectedChartInfo(bool playPreview = true);
    void updateChartTitleList();
    void resetSelectionIndices();
    
    void onSelectionChanged();
    void changeDifficulty(int direction);
    void handleLeftInput();
    void handleRightInput();
    void handleRateDecrease();
    void handleRateIncrease();
    void handleEnterInput();
    void setPackExpansion(bool isExpanded_, int packIndex);
    
    static const float LINE_SPACING;
};

#endif