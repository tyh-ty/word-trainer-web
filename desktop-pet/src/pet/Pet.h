#pragma once
#include "PetMood.h"
#include "PetStyle.h"
#include "../win/D2DRenderer.h"

enum class PetAction {
    Wave,
    Clap,
    Hop,
    Spin,
    Nod,
    Cheer
};

class Pet {
public:
    Pet(int size, float r, float g, float b);
    ~Pet();

    void update(float dt);
    void draw(D2DRenderer* renderer);

    void setMood(PetMood mood);
    PetMood getMood() const { return m_mood; }
    PetStyle getStyle() const { return m_style; }
    void onClick();
    void playAction(PetAction action);
    void setWalking(bool walking) { m_isWalking = walking; }
    void setColor(float r, float g, float b);
    void setStyle(PetStyle style);
    void setImageNailong(ID2D1Bitmap* bmp);
    void setImageAimee(ID2D1Bitmap* bmp);
    void setImageNailongShadow(ID2D1Bitmap* bmp);
    void setImageAimeeShadow(ID2D1Bitmap* bmp);

private:
    enum class SpriteAction {
        None,
        Wave,
        Clap,
        Hop,
        Spin,
        Nod,
        Cheer
    };

    void drawDefault(D2DRenderer* renderer, float cx, float cy, float s, float rx, float ry);
    void drawCharacterPet(D2DRenderer* renderer, float cx, float cy, float s, PetStyle style);
    void drawSpriteCharacter(D2DRenderer* renderer, ID2D1Bitmap* bitmap,
                             ID2D1Bitmap* shadowBitmap,
                             float cx, float cy, float s, PetStyle style,
                             float actionProgress, float envelope, float flutter);
    void drawCapsule(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                     float x1, float y1, float x2, float y2, float thickness);
    void drawCircle(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                    float cx, float cy, float radius);
    void drawAimeeCharacter(D2DRenderer* renderer, float cx, float cy, float s,
                            float actionProgress, float envelope, float flutter);
    void drawNailongCharacter(D2DRenderer* renderer, float cx, float cy, float s,
                              float actionProgress, float envelope, float flutter);
    void drawSpriteEffects(D2DRenderer* renderer, PetStyle style,
                           float cx, float cy, float s);
    void drawSparkle(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                     float x, float y, float radius, float alpha);
    void drawThoughtDots(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                         float x, float y, float scale, float alpha);
    void releaseBitmap(ID2D1Bitmap*& bmp);
    void startSpriteAction(SpriteAction action);

    void drawBody(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                  ID2D1SolidColorBrush* innerBrush, float cx, float cy, float rx, float ry);
    void drawEars(ID2D1RenderTarget* rt, ID2D1Factory* factory,
                  ID2D1SolidColorBrush* brush, ID2D1SolidColorBrush* innerBrush,
                  float cx, float cy, float s);
    void drawEyes(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* white,
                  ID2D1SolidColorBrush* dark, float cx, float cy, float s);
    void drawMouth(ID2D1RenderTarget* rt, ID2D1Factory* factory,
                   ID2D1SolidColorBrush* brush, float cx, float cy, float s);
    void drawBlush(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                   float cx, float cy, float s);
    void drawFeet(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                  float cx, float cy, float rx, float ry, float s);

    float m_animTime = 0;
    float m_bobOffset = 0;
    float m_breathPhase = 0;
    float m_blinkTimer = 0;
    float m_eyeScaleY = 1.0f;
    bool m_isBlinking = false;

    float m_jumpOffset = 0;
    float m_jumpVel = 0;
    bool m_isJumping = false;
    bool m_isWalking = false;
    SpriteAction m_action = SpriteAction::None;
    SpriteAction m_lastAction = SpriteAction::None;
    float m_actionTimer = 0.0f;
    float m_actionDir = 1.0f;
    float m_dancePhase = 0;
    float m_shakeOffset = 0;
    float m_swayOffset = 0;

    float m_idleTimer = 10.0f;
    float m_autoMoodTimer = 0;
    bool m_inAutoMood = false;

    PetMood m_mood = PetMood::Idle;
    PetStyle m_style = PetStyle::Default;

    ID2D1Bitmap* m_imgNailong = nullptr;
    ID2D1Bitmap* m_imgAimee = nullptr;
    ID2D1Bitmap* m_imgNailongShadow = nullptr;
    ID2D1Bitmap* m_imgAimeeShadow = nullptr;

    int m_size;
    float m_r, m_g, m_b;

    static constexpr float TWO_PI = 6.283185307f;
    static constexpr float BLINK_INTERVAL = 3.5f;
    static constexpr float BLINK_DURATION = 0.12f;
    static constexpr float BOB_SPEED = 2.2f;
    static constexpr float BOB_AMPLITUDE = 3.5f;
    static constexpr float JUMP_INIT_VEL = -170.0f;
    static constexpr float JUMP_GRAVITY = 480.0f;
    static constexpr float WAVE_DURATION = 1.25f;
    static constexpr float IDLE_INTERVAL_MIN = 8.0f;
    static constexpr float IDLE_INTERVAL_MAX = 18.0f;
    static constexpr float AUTO_MOOD_DURATION = 4.0f;
};
