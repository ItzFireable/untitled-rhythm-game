#ifndef RESULTS_STATE_H
#define RESULTS_STATE_H

#include <BaseState.h>
#include <objects/TextObject.h>
#include <vector>
#include <string>
#include <map>

class ResultsState : public BaseState
{
public:
    void init(AppContext* appContext, void* payload) override;
    void handleEvent(const SDL_Event& e) override;
    void render() override;
    void destroy() override;

    std::string getStateName() const override {
        return "ResultsState";
    }
private:
    TextObject* placeholderText_ = nullptr;
};

#endif