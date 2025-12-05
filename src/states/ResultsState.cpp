#include <states/ResultsState.h>

#include <objects/TextObject.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>

void ResultsState::init(AppContext* appContext, void* payload)
{
    BaseState::init(appContext, payload);
    
    this->placeholderText_ = new TextObject(renderer, MAIN_FONT_PATH, 24);
    this->placeholderText_->setAlignment(TEXT_ALIGN_CENTER);
    this->placeholderText_->setXAlignment(ALIGN_CENTER);
    this->placeholderText_->setYAlignment(ALIGN_MIDDLE);
    this->placeholderText_->setPosition(screenWidth_ / 2.0f, screenHeight_ / 2.0f);
    this->placeholderText_->setColor({255, 255, 255, 255});
    this->placeholderText_->setText("song done!!! press enter to go back to song select");
}

void ResultsState::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_RETURN:
                requestStateSwitch(STATE_SONG_SELECT, nullptr);
                break;
        }
    }
}

void ResultsState::render()
{
    if (this->placeholderText_) { this->placeholderText_->render(); }
}

void ResultsState::destroy()
{
    if (this->placeholderText_)
    {
        delete this->placeholderText_;
        this->placeholderText_ = nullptr;
    }
}