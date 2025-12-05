#ifndef NOTE_H
#define NOTE_H

#include <rhythm/Strum.h>
#include <rhythm/ChartManager.h>
#include <rhythm/JudgementSystem.h>

class Playfield;

class Note : public Strum
{
public:
    Note(float x, float y, float width, float height, int column = 0);
    ~Note();

    void update(float deltaTime) override;
    void render(SDL_Renderer *renderer) override;

    static void LoadSharedTexture(SDL_Renderer* renderer, Playfield* pf);
    static void DestroySharedTexture();

    void setTime(int t) { time = t; }
    void setType(NoteType t) { type = t; }
    void setColumn(int column) { column_ = column; }
    void setSpeedModifier(float speed) { speedModifier_ = speed; }

    float getTime() const { return time; }
    int getColumn() const { return column_; }
    NoteType getType() const { return type; }
    float getSpeedModifier() const { return speedModifier_; }

    bool canRenderNote() const { return canRender; }
    bool hasBeenHit() const { return hasBeenHitFlag; }
    
    virtual bool shouldDespawn() const { return despawned; }
    virtual void despawnNote() { despawned = true; };

    void markAsHit() { hasBeenHitFlag = true; }

    void setUpdatePos(bool update) { updatePos = update; }
    bool getUpdatePos() const { return updatePos; }

    void setAlphaMod(Uint8 alpha);
    SDL_Texture* getSharedTexture() const { return sharedTexture_; }
private:
    float time;
    int column_;
    NoteType type;

    bool updatePos = true;
    bool canRender = false;
    bool despawned = false;
    bool hasBeenHitFlag = false;
    float speedModifier_ = 1.0f;

    static SDL_Texture* sharedTexture_;
};

#endif