#ifndef RESULTS_STATE_H
#define RESULTS_STATE_H

#include <vector>
#include <string>
#include <map>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <utils/rhythm/ChartUtils.h>

#include <BaseState.h>
#include <objects/TextObject.h>
class ResultsState : public BaseState
{
public:
    void init(AppContext* appContext, void* payload) override;
    void handleEvent(const SDL_Event& e) override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

    std::string getStateName() const override {
        return "ResultsState";
    }
private:
    ResultsData* currentStateData_ = nullptr;
    SDL_Texture* backgroundTexture_ = nullptr;

    TextObject* chartInfoText_ = nullptr;
    TextObject* accuracyText_ = nullptr;
    TextObject* returnText_ = nullptr;
};

#endif