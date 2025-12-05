#include <states/SongSelectState.h>
#include <rhythm/ChartManager.h>
#include <rhythm/DifficultyCalculator.h>
#include <utils/Utils.h>
#include "utils/Logger.h"
#include <utils/AudioManager.h>
#include <objects/TextObject.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <rhythm/Conductor.h> 

static const float TITLE_X_POS = 16.0f;
static const float SCROLL_SPEED_PX_S = 200.0f;
static const float CROSSFADE_DURATION = 1.5f;
static const float LINE_SPACING = 30.0f;

std::string SongSelectState::getChartTitle(ChartData chartData) {
    auto it = chartData.metadata.find("title");
    if (it != chartData.metadata.end()) {
        return it->second;
    }
    return "Unknown Title";
}

std::string SongSelectState::getChartInfo(const ChartData& chartData) {
    std::stringstream ss;

    auto itArtist = chartData.metadata.find("artist");
    auto itTitle = chartData.metadata.find("title");
    auto itDifficulty = chartData.metadata.find("difficulty");

    ss << (itArtist != chartData.metadata.end() ? itArtist->second : "Unknown") << " - ";
    ss << (itTitle != chartData.metadata.end() ? itTitle->second : "Unknown") << "\n[";
    ss << (itDifficulty != chartData.metadata.end() ? itDifficulty->second : "Unknown") << " " << this->selectedRate << "x]";
    ss << " (" << std::fixed << std::setprecision(2) << conductor_->getCurrentBPM() << " BPM)";
    
    return ss.str();
}

void SongSelectState::init(AppContext* appContext, void* payload)
{
    BaseState::init(appContext, payload);
    
    this->selectedIndex_ = 0;
    this->listYOffset_ = 0.0f;
    this->targetYOffset_ = 0.0f;
    this->chartInfoText_ = nullptr;

    this->conductor_ = std::make_unique<Conductor>();

    std::vector<std::string> chartList = Utils::getChartList();
    
    for (const auto& chartPath : chartList)
    {
        std::string chartFile = Utils::getChartFile(chartPath);

        std::string fileContent = Utils::readFile(chartFile);
        ChartData chartData = ChartManager::parseChart(chartFile, fileContent);

        chartData.filePath = chartPath;
        this->charts_.push_back(chartData);
    }
    
    if (this->charts_.empty()) {
        GAME_LOG_ERROR("No charts loaded.");
        return;
    }

    this->listCenterX_ = screenWidth_ / 2.0f;
    this->listCenterY_ = screenHeight_ / 2.0f;

    for (const auto& chart : this->charts_) {
        TextObject* title = new TextObject(renderer, MAIN_FONT_PATH, 24);
        title->setText(this->getChartTitle(chart));
        title->setAlignment(TEXT_ALIGN_RIGHT);
        title->setXAlignment(ALIGN_RIGHT);
        title->setYAlignment(ALIGN_MIDDLE);
        title->setPosition(screenWidth_ - TITLE_X_POS, listCenterY_);
        this->chartTitles_.push_back(title);
    }

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

    conductor_->setOnSongEndCallback([this]() {
        this->playChartPreview(this->charts_[this->selectedIndex_]);
    });
    
    this->updateSelectedChartInfo();
    this->updateChartPositions();
}

void SongSelectState::playChartPreview(const ChartData& chartData)
{
    if (!conductor_) {
        GAME_LOG_ERROR("Conductor not initialized!");
        return;
    }
    
    std::string audioPath = Utils::getAudioPath(chartData);
    float previewTime = Utils::getAudioStartPos(chartData);
    float playbackRate = this->selectedRate; 

    if (!conductor_->initialize(chartData.timingPoints, audioPath, playbackRate, CROSSFADE_DURATION, previewTime)) {
        GAME_LOG_ERROR("Failed to initialize conductor for preview");
        return;
    }
}

void SongSelectState::updateSelectedChartInfo(bool playPreview)
{
    if (this->charts_.empty()) {
        return;
    }

    const ChartData& selectedChart = this->charts_[this->selectedIndex_];
    this->updateDifficultyDisplay(selectedChart);
    if (playPreview) {
        this->playChartPreview(selectedChart); 
    }

    this->chartInfoText_->setText(this->getChartInfo(selectedChart));
}

void SongSelectState::updateChartPositions()
{
    for (size_t i = 0; i < this->chartTitles_.size(); ++i) {
        float yPos = (screenHeight_ / 2.0f) + (float)i * LINE_SPACING + this->listYOffset_;
        this->chartTitles_[i]->setPosition(screenWidth_ - TITLE_X_POS, yPos);
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

void SongSelectState::updateDifficultyDisplay(ChartData chartData)
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

void SongSelectState::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_UP:
                this->selectedIndex_ = (this->selectedIndex_ - 1 + this->charts_.size()) % this->charts_.size();
                this->targetYOffset_ = -this->selectedIndex_ * LINE_SPACING;
                this->updateSelectedChartInfo();
                break;
            case SDLK_DOWN:
                this->selectedIndex_ = (this->selectedIndex_ + 1) % this->charts_.size();
                this->targetYOffset_ = -this->selectedIndex_ * LINE_SPACING;
                this->updateSelectedChartInfo();
                break;
            case SDLK_LEFT:
                this->selectedRate = std::max(0.1f, this->selectedRate - 0.1f);
                this->updateDifficultyDisplay(this->charts_[this->selectedIndex_]);
                conductor_->setPlaybackRate(this->selectedRate);
                this->updateSelectedChartInfo(false);
                break;
            case SDLK_RIGHT:
                this->selectedRate = std::min(5.0f, this->selectedRate + 0.1f);
                this->updateDifficultyDisplay(this->charts_[this->selectedIndex_]);
                conductor_->setPlaybackRate(this->selectedRate);
                this->updateSelectedChartInfo(false);
                break;
            case SDLK_RETURN:
                if (!this->charts_.empty()) {
                    PlayStateData* payload = new PlayStateData();
                    payload->chartData = this->charts_[this->selectedIndex_];
                    payload->songPath =  this->charts_[this->selectedIndex_].filePath;
                    payload->chartFile = this->charts_[this->selectedIndex_].filename;
                    payload->playbackRate = this->selectedRate;

                    if (conductor_) {
                        conductor_->stop();
                    }
                    
                    requestStateSwitch(STATE_PLAY, payload);
                }
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
        float moveAmount = SCROLL_SPEED_PX_S * deltaTime;
        
        if (distance > 0) {
            this->listYOffset_ += std::min(moveAmount, distance);
        } else {
            this->listYOffset_ -= std::min(moveAmount, -distance); 
        }
    }
    
    this->updateChartPositions();

    for (int i = 0; i < this->chartTitles_.size(); ++i)
    {
        if (i == this->selectedIndex_) {
            this->chartTitles_[i]->setColor({255, 255, 0, 255});
        } else {
            this->chartTitles_[i]->setColor({255, 255, 255, 255});
        }
    }

    if (conductor_) {
        conductor_->update(deltaTime);
    }
}

void SongSelectState::render()
{
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
        this->diffTextObject_->render();
    }
}

void SongSelectState::destroy()
{
    if (conductor_) {
        conductor_->stop();
        conductor_.reset();
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
}