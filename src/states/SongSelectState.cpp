#include <states/SongSelectState.h>
#include <utils/rhythm/ChartUtils.h>
#include <rhythm/DifficultyCalculator.h>
#include <utils/Utils.h>
#include "system/Logger.h"
#include <system/AudioManager.h>
#include <objects/TextObject.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <rhythm/Conductor.h> 
#include <filesystem>
#include <algorithm>

static const float TITLE_X_POS = 16.0f;
static const float SCROLL_SPEED_PX_S = 200.0f;
static const float CROSSFADE_DURATION = 1.5f;
static const float LINE_SPACING = 30.0f;
static const float SONG_INDENT = 32.0f;

const float SongSelectState::LINE_SPACING = 30.0f;

void SongSelectState::loadAndCrossfadeBackground(const ChartData& chartData)
{
    std::string bgName = ChartUtils::getChartBackgroundName(chartData);
    std::string newIdentifier = chartData.filePath + "/" + bgName; 
    if (newIdentifier == this->lastLoadedChartIdentifier_) {
        return;
    }
    this->lastLoadedChartIdentifier_ = newIdentifier;

    if (this->nextBackgroundTexture_) {
        SDL_DestroyTexture(this->nextBackgroundTexture_);
        this->nextBackgroundTexture_ = nullptr;
    }
    this->crossfadeTimer_ = 0.0f;

    SDL_Texture* newTexture = nullptr;
    if (!bgName.empty())
    {
        std::string bgPath = chartData.filePath + "/" + bgName;
        newTexture = IMG_LoadTexture(renderer, bgPath.c_str());
        if (newTexture) {
            SDL_SetTextureColorMod(newTexture, 77, 77, 77);
            SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
        }
    }
    
    this->isFadingOut_ = (newTexture == nullptr && this->currentBackgroundTexture_ != nullptr);
    this->nextBackgroundTexture_ = newTexture;
}
std::string SongSelectState::getChartTitle(const ChartData& chartData) {
    auto it = chartData.metadata.find("title");
    if (it != chartData.metadata.end()) {
        return it->second;
    }
    return "Unknown Title";
}

SongEntry SongSelectState::loadSongEntry(const std::string& songDirectory, const std::vector<std::string>& chartFiles) 
{
    SongEntry song;
    song.folderPath = songDirectory;

    for (const auto& chartFile : chartFiles)
    {
        std::string fileContent = Utils::readFile(chartFile);
        
        std::map<std::string, ChartData> chartsFromFile = ChartUtils::parseChartMultiple(songDirectory, chartFile, fileContent);
        
        for (auto& [difficultyName, chartData] : chartsFromFile)
        {
            chartData.filePath = songDirectory;
            chartData.filename = chartFile;

            if (song.difficulties.empty()) {
                auto itTitle = chartData.metadata.find("title");
                auto itArtist = chartData.metadata.find("artist");
                song.title = (itTitle != chartData.metadata.end() ? itTitle->second : "Unknown Title");
                song.artist = (itArtist != chartData.metadata.end() ? itArtist->second : "Unknown Artist");
            }
            
            song.difficulties[difficultyName] = chartData;
        }
    }
    
    return song;
}

void SongSelectState::buildFlatSongList()
{
    this->flatSongList_.clear();
    
    for (size_t packIdx = 0; packIdx < this->songPacks_.size(); ++packIdx) {
        const SongPack& pack = this->songPacks_[packIdx];
        
        FlatSongEntry packEntry;
        packEntry.isPackHeader = true;
        packEntry.packIndex = packIdx;
        packEntry.songIndex = -1;
        packEntry.displayText = pack.name;
        this->flatSongList_.push_back(packEntry);
        
        if (pack.isExpanded) {
            for (size_t songIdx = 0; songIdx < pack.songs.size(); ++songIdx) {
                const SongEntry& song = pack.songs[songIdx];
                
                FlatSongEntry songEntry;
                songEntry.isPackHeader = false;
                songEntry.packIndex = packIdx;
                songEntry.songIndex = songIdx;
                
                std::string difficultyName = song.difficulties.begin()->first;
                if (!this->currentSelectedDifficultyName_.empty() && 
                    song.difficulties.find(this->currentSelectedDifficultyName_) != song.difficulties.end()) {
                    difficultyName = this->currentSelectedDifficultyName_;
                }
                
                songEntry.displayText = song.title;
                this->flatSongList_.push_back(songEntry);
            }
        }
    }
}

void SongSelectState::updateChartTitleList()
{
    for (TextObject *title : this->chartTitles_)
    {
        delete title;
    }
    this->chartTitles_.clear();

    if (this->flatSongList_.empty()) return;
    
    for (const auto& entry : this->flatSongList_) {
        TextObject* title = new TextObject(renderer, MAIN_FONT_PATH, entry.isPackHeader ? 32 : 24);
        title->setText(entry.displayText);
        title->setAlignment(TEXT_ALIGN_RIGHT);
        title->setXAlignment(ALIGN_RIGHT);
        title->setYAlignment(ALIGN_MIDDLE);
        
        float xPos = screenWidth_ - TITLE_X_POS;
        
        title->setPosition(xPos, listCenterY_);
        this->chartTitles_.push_back(title);
    }
}

void SongSelectState::resetSelectionIndices()
{
    this->selectedIndex_ = 0; 
    this->listYOffset_ = 0.0f;
    this->targetYOffset_ = 0.0f;
    this->lastSelectedIndex_ = -1; 
    this->currentSelectedDifficultyName_ = "";
}

void SongSelectState::init(AppContext* appContext, void* payload)
{
    BaseState::init(appContext, payload);

    this->listCenterX_ = screenWidth_ / 2.0f;
    this->listCenterY_ = screenHeight_ / 2.0f;
    this->conductor_ = std::make_unique<Conductor>();
    
    conductorInfo_ = new ConductorInfo(renderer, MAIN_FONT_PATH, 16, 7.0f);
    conductorInfo_->setConductor(conductor_.get());

    DebugInfo* debugInfo = appContext->debugInfo;
    FPSCounter* fpsCounter = appContext->fpsCounter;

    if (debugInfo)
    {
        conductorInfo_->setYPosition(debugInfo->getYPosition() + debugInfo->getHeight() + 8.0f);
    }
    else if (fpsCounter)
    {
        conductorInfo_->setYPosition(fpsCounter->getYPosition() + fpsCounter->getHeight() + 4.0f);
    }

    std::vector<std::string> topLevelFolders = Utils::getChartList();
    SongPack miscellaneousPack;
    miscellaneousPack.name = "Miscellaneous";

    for (const auto& folderPath : topLevelFolders)
    {
        std::vector<std::string> chartFiles = Utils::getChartFiles(folderPath);

        if (!chartFiles.empty()) {
            SongEntry song = loadSongEntry(folderPath, chartFiles);
            miscellaneousPack.songs.push_back(song);
        } else {
            SongPack newPack;
            newPack.name = std::filesystem::path(folderPath).filename().string();
            
            for (const auto &subEntry : std::filesystem::directory_iterator(folderPath))
            {
                if (subEntry.is_directory())
                {
                    std::string songFolderPath = subEntry.path().string();
                    std::vector<std::string> songChartFiles = Utils::getChartFiles(songFolderPath);
                    
                    if (!songChartFiles.empty()) {
                        SongEntry song = loadSongEntry(songFolderPath, songChartFiles);
                        newPack.songs.push_back(song);
                    }
                }
            }
            
            if (!newPack.songs.empty()) {
                this->songPacks_.push_back(newPack);
            }
        }
    }
    
    if (!miscellaneousPack.songs.empty()) {
        this->songPacks_.push_back(miscellaneousPack);
    }
    
    if (this->songPacks_.empty()) {
        GAME_LOG_ERROR("No charts or packs loaded.");
        return;
    }
    
    this->buildFlatSongList();
    this->resetSelectionIndices();

    this->diffTextObject_ = new TextObject(this->renderer, MAIN_FONT_PATH, 16);
    this->diffTextObject_->setAlignment(TEXT_ALIGN_LEFT);
    this->diffTextObject_->setXAlignment(ALIGN_LEFT);
    this->diffTextObject_->setYAlignment(ALIGN_MIDDLE);
    this->diffTextObject_->setPosition(16.0f, listCenterY_);
    this->diffTextObject_->setColor({255, 255, 255, 255});
    
    this->chartInfoText_ = new TextObject(renderer, MAIN_FONT_PATH, 16);
    this->chartInfoText_->setAlignment(TEXT_ALIGN_CENTER);
    this->chartInfoText_->setXAlignment(ALIGN_CENTER);
    this->chartInfoText_->setYAlignment(ALIGN_TOP);
    this->chartInfoText_->setPosition(listCenterX_, 16.0f);
    this->chartInfoText_->setColor({255, 255, 255, 255});

    conductor_->setOnBPMChangeCallback([this](float newBPM) {
        this->updateSelectedChartInfo(false);
    });

    /*conductor_->setOnSongEndCallback([this]() {
        this->playChartPreview(this->getCurrentSelectedChart());
    });*/
    
    this->updateChartTitleList();
    if (payload) {
        SongSelectData* data = static_cast<SongSelectData*>(payload);

        this->selectedRate = data->previousRate;
        int targetPackIndex = -1;

        for (size_t i = 0; i < this->songPacks_.size(); ++i) {
            if (this->songPacks_[i].name == data->previousPackName) {
                this->songPacks_[i].isExpanded = true;
                targetPackIndex = i;
                break;
            }
        }

        this->buildFlatSongList();
        this->updateChartTitleList();

        int targetIndex = data->previousIndex; 
        if (targetIndex >= 0 && targetIndex < this->flatSongList_.size()) {
            this->selectedIndex_ = targetIndex;

            this->listYOffset_ = -this->selectedIndex_ * LINE_SPACING; 
            this->targetYOffset_ = this->listYOffset_;
            this->lastSelectedIndex_ = targetIndex;
        } else {
            this->selectedIndex_ = 0;
            this->listYOffset_ = 0.0f;
            this->targetYOffset_ = 0.0f;
            this->lastSelectedIndex_ = -1;
        }

        delete data;
    }

    this->updateSelectedChartInfo();
    this->updateChartPositions();
}

ChartData& SongSelectState::getCurrentSelectedChart()
{
    const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
    SongPack& pack = this->songPacks_[entry.packIndex];
    SongEntry& song = pack.songs[entry.songIndex];
    
    if (this->currentSelectedDifficultyName_.empty()) {
        this->currentSelectedDifficultyName_ = song.difficulties.begin()->first;
    }
    
    return song.difficulties.at(this->currentSelectedDifficultyName_);
}

const ChartData& SongSelectState::getCurrentSelectedChart() const
{
    const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
    const SongPack& pack = this->songPacks_[entry.packIndex];
    const SongEntry& song = pack.songs[entry.songIndex];
    
    std::string diffName = this->currentSelectedDifficultyName_.empty() 
        ? song.difficulties.begin()->first 
        : this->currentSelectedDifficultyName_;
    
    return song.difficulties.at(diffName);
}

void SongSelectState::playChartPreview(const ChartData& chartData)
{
    if (!conductor_) {
        GAME_LOG_ERROR("Conductor not initialized!");
        return;
    }
    
    std::string audioPath = Utils::getAudioPath(chartData);
    float previewTime = Utils::getAudioStartPos(chartData);
    float previewDuration = Utils::getAudioPreviewLength(chartData);
    float playbackRate = this->selectedRate;

    conductor_->setLooping(true);
    conductor_->setLoopRegion(previewTime, previewDuration > 0.0f ? previewTime + previewDuration : -1.0f);
    conductor_->setLoopFadeOutDuration(2.0f);
    conductor_->setLoopPauseDuration(1.5f); 

    if (!conductor_->initialize(chartData.timingPoints, audioPath, playbackRate, CROSSFADE_DURATION, previewTime)) {
        GAME_LOG_ERROR("Failed to initialize conductor for preview");
        return;
    }
}

void SongSelectState::updateSelectedChartInfo(bool playPreview)
{
    if (this->flatSongList_.empty()) {
        return;
    }

    const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
    
    if (entry.isPackHeader) {
        this->chartInfoText_->setText(this->songPacks_[entry.packIndex].name);
        if (this->diffTextObject_) this->diffTextObject_->setText("");
        if (conductor_) conductor_->stop();

        this->loadAndCrossfadeBackground(ChartData());
        return;
    }

    ChartData& selectedChart = this->getCurrentSelectedChart();
    this->updateDifficultyDisplay(selectedChart);
    
    if (playPreview) {
        this->loadAndCrossfadeBackground(selectedChart);
        this->playChartPreview(selectedChart); 
    }

    this->chartInfoText_->setText(ChartUtils::getChartInfo(selectedChart, this->selectedRate));
}

void SongSelectState::updateChartPositions()
{
    for (size_t i = 0; i < this->chartTitles_.size(); ++i) {
        float yPos = (screenHeight_ / 2.0f) + (float)i * LINE_SPACING + this->listYOffset_;
        
        const FlatSongEntry& entry = this->flatSongList_[i];
        float xPos = screenWidth_ - TITLE_X_POS;
        this->chartTitles_[i]->setPosition(xPos, yPos);
    }
}

void updateDifficultyText(TextObject* textObject, const FinalResult& result)
{
    if (!textObject) return;

    std::stringstream ss;

    ss << std::fixed;
    ss << std::setprecision(3);
    
    ss << "Raw Diff (MSD): " << result.rawDiff << " (" << result.playbackRate << "x)\n";
    ss << "Total Rice Skill: " << result.riceTotal << "\n";
    ss << "Total LN Skill: " << result.lnTotal << "\n";

    ss << "\n+ Skill Breakdown (Rice)\n";
    ss << "Stream: " << result.skills.stream << "\n";
    ss << "Jumpstream: " << result.skills.jumpstream << "\n";
    ss << "Handstream: " << result.skills.handstream << "\n";
    ss << "Jack: " << result.skills.jack << "\n";
    ss << "Chordjack: " << result.skills.chordjack << "\n";
    ss << "Technical: " << result.skills.technical << "\n";
    ss << "Stamina: " << result.skills.stamina << "\n";

    ss << "\n+ Skill Breakdown (Long Notes)\n";
    ss << "LN Density: " << result.skills.density << "\n";
    ss << "LN Speed: " << result.skills.speed << "\n";
    ss << "LN Shields: " << result.skills.shields << "\n";
    ss << "LN Complexity: " << result.skills.complexity;

    textObject->setText(ss.str());
}

void SongSelectState::updateDifficultyDisplay(ChartData& chartData)
{
    FinalResult result;
    const std::string& key = chartData.filename + "@" + std::to_string(this->selectedRate);

    if (this->difficultyCache_.count(key)) {
        result = this->difficultyCache_[key];
    } else {
        result = this->calculator.calculate(chartData, this->selectedRate);
        this->difficultyCache_[key] = result;
    }

    updateDifficultyText(this->diffTextObject_, result);
}

void SongSelectState::onSelectionChanged()
{
    if (this->flatSongList_.empty()) return;
    
    const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
    
    if (!entry.isPackHeader) {
        const SongPack& pack = this->songPacks_[entry.packIndex];
        const SongEntry& song = pack.songs[entry.songIndex];
        
        if (this->currentSelectedDifficultyName_.empty() || 
            song.difficulties.find(this->currentSelectedDifficultyName_) == song.difficulties.end()) {
            this->currentSelectedDifficultyName_ = song.difficulties.begin()->first;
        }
    }
    
    this->lastInputTime_ = SDL_GetTicks();
    this->updateSelectedChartInfo(false);
}

void SongSelectState::changeDifficulty(int direction)
{
    if (this->flatSongList_.empty()) return;
    
    const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
    if (entry.isPackHeader) return;
    
    const SongPack& pack = this->songPacks_[entry.packIndex];
    const SongEntry& song = pack.songs[entry.songIndex];
    if (song.difficulties.empty()) return;
    
    auto it = song.difficulties.find(this->currentSelectedDifficultyName_);
    if (it == song.difficulties.end()) {
        it = song.difficulties.begin();
    }
    
    if (direction < 0) {
        if (it == song.difficulties.begin()) {
            it = std::prev(song.difficulties.end());
        } else {
            it = std::prev(it);
        }
    } else {
        it = std::next(it);
        if (it == song.difficulties.end()) {
            it = song.difficulties.begin();
        }
    }
    
    this->currentSelectedDifficultyName_ = it->first;
    this->updateSelectedChartInfo(true);
}

void SongSelectState::handleLeftInput()
{
    if (this->flatSongList_.empty()) return;
    
    const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
    if (!entry.isPackHeader) {
        this->changeDifficulty(-1);
    }
}

void SongSelectState::handleRightInput()
{
    if (this->flatSongList_.empty()) return;
    
    const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
    if (!entry.isPackHeader) {
        this->changeDifficulty(1);
    }
}

void SongSelectState::handleRateDecrease()
{
    this->selectedRate = std::max(0.1f, this->selectedRate - 0.1f);
    
    if (!this->flatSongList_.empty()) {
        const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
        if (!entry.isPackHeader) {
            this->updateDifficultyDisplay(this->getCurrentSelectedChart());
        }
    }
    
    if (conductor_) {
        conductor_->setPlaybackRate(this->selectedRate);
    }
    this->updateSelectedChartInfo(false);
}

void SongSelectState::handleRateIncrease()
{
    this->selectedRate = std::min(5.0f, this->selectedRate + 0.1f);
    
    if (!this->flatSongList_.empty()) {
        const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
        if (!entry.isPackHeader) {
            this->updateDifficultyDisplay(this->getCurrentSelectedChart());
        }
    }
    
    if (conductor_) {
        conductor_->setPlaybackRate(this->selectedRate);
    }
    this->updateSelectedChartInfo(false);
}

void SongSelectState::setPackExpansion(bool isExpanded_, int packIndex)
{
    if (this->flatSongList_.empty()) return;
    
    SongPack& pack = this->songPacks_[packIndex];
    pack.isExpanded = isExpanded_;
    
    this->buildFlatSongList();
    this->updateChartTitleList();
    
    int newSelectedIndex = -1;
    
    for (size_t i = 0; i < this->flatSongList_.size(); ++i) {
        const FlatSongEntry& entry = this->flatSongList_[i];
        if (entry.isPackHeader && entry.packIndex == packIndex) {
            newSelectedIndex = i;
            break;
        }
    }
    
    if (newSelectedIndex != -1) {
        this->selectedIndex_ = newSelectedIndex;
        this->targetYOffset_ = -this->selectedIndex_ * LINE_SPACING;
        this->listYOffset_ = this->targetYOffset_; 
        this->lastSelectedIndex_ = newSelectedIndex;
    }
    
    this->updateSelectedChartInfo(false);
}
void SongSelectState::handleEnterInput()
{
    if (this->flatSongList_.empty()) return;
    
    const FlatSongEntry& entry = this->flatSongList_[this->selectedIndex_];
    
    if (entry.isPackHeader) {
        int currentPackIndex = entry.packIndex;

        if (!this->songPacks_[entry.packIndex].isExpanded) {
            for (size_t i = 0; i < this->songPacks_.size(); ++i) {
                if (i != entry.packIndex) {
                    this->songPacks_[i].isExpanded = false;
                }
            }
        }

        this->setPackExpansion(!this->songPacks_[entry.packIndex].isExpanded, currentPackIndex);
        return;
    }
    
    ChartData& selectedChart = this->getCurrentSelectedChart();
    PlayStateData* payload = new PlayStateData();
    payload->chartData = selectedChart;
    payload->songPath = selectedChart.filePath;
    payload->chartFile = selectedChart.filename;
    payload->playbackRate = this->selectedRate;

    payload->previousStateData = new SongSelectData{
        this->selectedIndex_,
        this->songPacks_[entry.packIndex].name,
        this->songPacks_[entry.packIndex].songs[entry.songIndex].title,
        this->selectedRate
    };

    if (conductor_) {
        conductor_->stop();
    }
    
    requestStateSwitch(STATE_PLAY, payload);
}

void SongSelectState::handleEvent(const SDL_Event& event)
{
    if (this->flatSongList_.empty()) return;

    int listSize = this->chartTitles_.size(); 
    
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_UP:
                if (listSize > 0) {
                    this->selectedIndex_ = (this->selectedIndex_ - 1 + listSize) % listSize;
                    this->targetYOffset_ = -this->selectedIndex_ * LINE_SPACING;
                    this->onSelectionChanged();
                }
                break;
                
            case SDLK_DOWN:
                if (listSize > 0) {
                    this->selectedIndex_ = (this->selectedIndex_ + 1) % listSize;
                    this->targetYOffset_ = -this->selectedIndex_ * LINE_SPACING;
                    this->onSelectionChanged();
                }
                break;
                
            case SDLK_LEFT:
                this->handleLeftInput();
                break;
                
            case SDLK_RIGHT:
                this->handleRightInput();
                break;
                
            case SDLK_MINUS:
            case SDLK_KP_MINUS:
                this->handleRateDecrease();
                break;
                
            case SDLK_EQUALS:
            case SDLK_PLUS:
            case SDLK_KP_PLUS:
                this->handleRateIncrease();
                break;
                
            case SDLK_RETURN:
                this->handleEnterInput();
                break;
        }
    }
}

void SongSelectState::update(float deltaTime)
{
    float distance = this->targetYOffset_ - this->listYOffset_;

    if (std::abs(distance) < 1.0f) 
    {
        this->listYOffset_ = this->targetYOffset_;
    } 
    else 
    {
        float smoothing = 10.0f;
        float t = 1.0f - std::exp(-smoothing * deltaTime);
        this->listYOffset_ += distance * t;
    }

    if (this->nextBackgroundTexture_ || this->isFadingOut_) {
        this->crossfadeTimer_ += deltaTime;
        
        if (this->crossfadeTimer_ >= CROSSFADE_DURATION / 4.0f) {
            this->crossfadeTimer_ = 0.0f;
            
            if (this->currentBackgroundTexture_) {
                SDL_DestroyTexture(this->currentBackgroundTexture_);
            }
            
            this->currentBackgroundTexture_ = this->nextBackgroundTexture_;
            this->nextBackgroundTexture_ = nullptr;
            
            if (this->currentBackgroundTexture_) {
                SDL_SetTextureAlphaMod(this->currentBackgroundTexture_, 255);
            }
        }
    }

    if (this->selectedIndex_ != this->lastSelectedIndex_) 
    {
        Uint64 currentTime = SDL_GetTicks();
        
        if (currentTime - this->lastInputTime_ >= this->PREVIEW_DEBOUNCE_MS) 
        {
            this->updateSelectedChartInfo();
            this->lastSelectedIndex_ = this->selectedIndex_; 
        }
    }

    if (this->selectedIndex_ != this->lastSelectedIndex_) 
    {
        Uint64 currentTime = SDL_GetTicks();
        
        if (currentTime - this->lastInputTime_ >= this->PREVIEW_DEBOUNCE_MS) 
        {
            this->updateSelectedChartInfo();
            this->lastSelectedIndex_ = this->selectedIndex_; 
        }
    }
    
    this->updateChartPositions();

    for (int i = 0; i < this->chartTitles_.size(); ++i)
    {
        if (!this->flatSongList_.empty() && this->flatSongList_[i].isPackHeader) {
            const SongPack& pack = this->songPacks_[this->flatSongList_[i].packIndex];
            if (pack.isExpanded) {
                this->chartTitles_[i]->setColor({255, 255, 0, 255});
            } else {
                this->chartTitles_[i]->setColor({255, 255, 255, 255});
            }
        }
        
        if (i == this->selectedIndex_) {
            this->chartTitles_[i]->setColor({255, 255, 0, 255});
        } else if (!this->flatSongList_.empty() && !this->flatSongList_[i].isPackHeader) {
            this->chartTitles_[i]->setColor({255, 255, 255, 255});
        }
    }

    if (conductor_) {
        conductor_->update(deltaTime);

        if (conductorInfo_)
        {
            DebugInfo* debugInfo = appContext->debugInfo;
            FPSCounter* fpsCounter = appContext->fpsCounter;

            if (debugInfo)
            {
                conductorInfo_->setYPosition(debugInfo->getYPosition() + debugInfo->getHeight() + 8.0f);
            }
            else if (fpsCounter)
            {
                conductorInfo_->setYPosition(fpsCounter->getYPosition() + fpsCounter->getHeight() + 4.0f);
            }
            conductorInfo_->update();
        }
    }
}

void SongSelectState::render()
{
    bool isFading = (this->nextBackgroundTexture_ != nullptr) || (this->crossfadeTimer_ > 0.0f) || this->isFadingOut_;
    float fadeProgress = std::min(1.0f, this->crossfadeTimer_ / (CROSSFADE_DURATION / 4.0f));
    
    if (this->currentBackgroundTexture_ && !isFading)
    {
        SDL_SetTextureAlphaMod(this->currentBackgroundTexture_, 255);
        SDL_RenderTexture(renderer, this->currentBackgroundTexture_, nullptr, nullptr);
    }

    if (this->currentBackgroundTexture_ && isFading)
    {
        Uint8 alpha = (Uint8)((1.0f - fadeProgress) * 255.0f);

        if (alpha > 0) {
            SDL_SetTextureAlphaMod(this->currentBackgroundTexture_, alpha);
            SDL_RenderTexture(renderer, this->currentBackgroundTexture_, nullptr, nullptr);
        }
    }

    if (this->nextBackgroundTexture_)
    {
        Uint8 alpha = (Uint8)(fadeProgress * 255.0f);
        
        if (alpha > 0) {
            SDL_SetTextureAlphaMod(this->nextBackgroundTexture_, alpha);
            SDL_RenderTexture(renderer, this->nextBackgroundTexture_, nullptr, nullptr);
        }
    }

    for (TextObject* title : this->chartTitles_)
    {
        title->render();
    }
    
    if (this->chartInfoText_)
    {
        this->chartInfoText_->render();
    }

    if (this->diffTextObject_)
    {
        //this->diffTextObject_->render();
    }

    if (conductorInfo_) { conductorInfo_->render(renderer);}
}

void SongSelectState::destroy()
{
    if (conductorInfo_)
    {
        delete conductorInfo_;
        conductorInfo_ = nullptr;
    }

    if (conductor_) {
        conductor_->stop();
        conductor_.reset();
    }

    if (this->currentBackgroundTexture_) {
        SDL_DestroyTexture(this->currentBackgroundTexture_);
        this->currentBackgroundTexture_ = nullptr;
    }
    if (this->nextBackgroundTexture_) {
        SDL_DestroyTexture(this->nextBackgroundTexture_);
        this->nextBackgroundTexture_ = nullptr;
    }
    
    for (TextObject *title : this->chartTitles_)
    {
        delete title;
    }
    this->chartTitles_.clear();

    if (this->chartInfoText_)
    {
        delete this->chartInfoText_;
        this->chartInfoText_ = nullptr;
    }

    if (this->diffTextObject_)
    {
        delete this->diffTextObject_;
        this->diffTextObject_ = nullptr;
    }
}