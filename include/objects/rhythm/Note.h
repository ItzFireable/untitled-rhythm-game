#ifndef NOTE_H
#define NOTE_H

#include <objects/rhythm/Strum.h>
#include <rhythm/JudgementSystem.h>
#include <utils/rhythm/ChartUtils.h>

class Playfield;

class Note : public Strum
{
public:
    Note(float x, float y, float width, float height, int column = 0);
    ~Note();

    void update(float deltaTime) override;
    void render(SDL_Renderer *renderer) override;
    
    void loadTextures(SDL_Renderer* renderer, Playfield* pf) override;
    void destroyTextures() override;

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

private:
    float time;
    int column_;
    NoteType type;

    bool updatePos = true;
    bool canRender = false;
    bool despawned = false;
    bool hasBeenHitFlag = false;
    float speedModifier_ = 1.0f;
    
    static SDL_Texture* sharedNoteTexture_;
    static SDL_Texture* sharedMineTexture_;
};

#endif