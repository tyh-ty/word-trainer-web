#include "Pet.h"
#include "../util/SoundPlayer.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace {

float clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

}

Pet::Pet(int size, float r, float g, float b)
    : m_size(size), m_r(r), m_g(g), m_b(b) {
    std::srand((unsigned)std::time(nullptr));
}

Pet::~Pet() {
    releaseBitmap(m_imgNailong);
    releaseBitmap(m_imgAimee);
    releaseBitmap(m_imgNailongShadow);
    releaseBitmap(m_imgAimeeShadow);
}

void Pet::releaseBitmap(ID2D1Bitmap*& bmp) {
    if (bmp) {
        bmp->Release();
        bmp = nullptr;
    }
}

void Pet::setImageNailong(ID2D1Bitmap* bmp) {
    if (m_imgNailong == bmp) return;
    releaseBitmap(m_imgNailong);
    m_imgNailong = bmp;
}

void Pet::setImageAimee(ID2D1Bitmap* bmp) {
    if (m_imgAimee == bmp) return;
    releaseBitmap(m_imgAimee);
    m_imgAimee = bmp;
}

void Pet::setImageNailongShadow(ID2D1Bitmap* bmp) {
    if (m_imgNailongShadow == bmp) return;
    releaseBitmap(m_imgNailongShadow);
    m_imgNailongShadow = bmp;
}

void Pet::setImageAimeeShadow(ID2D1Bitmap* bmp) {
    if (m_imgAimeeShadow == bmp) return;
    releaseBitmap(m_imgAimeeShadow);
    m_imgAimeeShadow = bmp;
}

void Pet::update(float dt) {
    m_animTime += dt;

    float bobSpeed = m_isWalking ? BOB_SPEED * 3.0f : BOB_SPEED;
    float bobAmp = m_isWalking ? BOB_AMPLITUDE * 2.5f : BOB_AMPLITUDE;
    m_breathPhase = std::sin(m_animTime * bobSpeed);
    m_bobOffset = m_breathPhase * bobAmp;

    m_dancePhase += dt;
    float sway = 0.0f;
    if (m_mood == PetMood::Happy) {
        sway = std::sin(m_dancePhase * 3.0f) * 5.0f;
    } else if (m_mood == PetMood::Curious) {
        sway = std::sin(m_dancePhase * 1.8f) * 2.5f;
    } else if (m_mood == PetMood::Sleepy) {
        sway = std::sin(m_dancePhase * 0.8f) * 1.2f;
    }

    if (m_shakeOffset != 0.0f) {
        m_shakeOffset *= 0.85f;
        if (std::abs(m_shakeOffset) < 0.3f) m_shakeOffset = 0.0f;
    }
    m_swayOffset = sway + m_shakeOffset;

    m_blinkTimer += dt;
    if (!m_isBlinking) {
        if (m_blinkTimer > BLINK_INTERVAL) {
            m_isBlinking = true;
            m_blinkTimer = 0.0f;
        }
        m_eyeScaleY = 1.0f;
    } else {
        float t = m_blinkTimer / BLINK_DURATION;
        if (t >= 1.0f) {
            m_isBlinking = false;
            m_blinkTimer = 0.0f;
            m_eyeScaleY = 1.0f;
        } else {
            m_eyeScaleY = (1.0f + std::cos(t * TWO_PI)) * 0.5f;
        }
    }

    if (m_isJumping) {
        m_jumpVel += JUMP_GRAVITY * dt;
        m_jumpOffset += m_jumpVel * dt;
        if (m_jumpOffset >= 0.0f) {
            m_jumpOffset = 0.0f;
            m_jumpVel = 0.0f;
            m_isJumping = false;
        }
    }

    if (m_action != SpriteAction::None) {
        m_actionTimer += dt;
        float actionDuration = 1.0f;
        switch (m_action) {
        case SpriteAction::Wave:  actionDuration = 1.25f; break;
        case SpriteAction::Clap:  actionDuration = 1.00f; break;
        case SpriteAction::Hop:   actionDuration = 0.95f; break;
        case SpriteAction::Spin:  actionDuration = 1.20f; break;
        case SpriteAction::Nod:   actionDuration = 0.85f; break;
        case SpriteAction::Cheer: actionDuration = 1.10f; break;
        default:                  actionDuration = 1.0f;  break;
        }
        if (m_actionTimer >= actionDuration) {
            m_action = SpriteAction::None;
            m_actionTimer = 0.0f;
        }
    }

    if (m_inAutoMood) {
        m_autoMoodTimer -= dt;
        if (m_autoMoodTimer <= 0.0f) {
            m_mood = PetMood::Idle;
            m_inAutoMood = false;
            float range = IDLE_INTERVAL_MAX - IDLE_INTERVAL_MIN;
            m_idleTimer = IDLE_INTERVAL_MIN + (float)std::rand() / RAND_MAX * range;
        }
    } else {
        m_idleTimer -= dt;
        if (m_idleTimer <= 0.0f) {
            static const PetMood moods[] = {
                PetMood::Happy, PetMood::Curious, PetMood::Sleepy,
                PetMood::Surprised, PetMood::Thinking
            };
            int idx = std::rand() % 5;
            m_mood = moods[idx];
            m_inAutoMood = true;
            m_autoMoodTimer = AUTO_MOOD_DURATION;

            if (m_action == SpriteAction::None && (std::rand() % 3) == 0) {
                static const SpriteAction idleActions[] = {
                    SpriteAction::Nod,
                    SpriteAction::Wave,
                    SpriteAction::Cheer
                };
                int actionCount = (int)(sizeof(idleActions) / sizeof(idleActions[0]));
                int actionIndex = std::rand() % actionCount;
                SpriteAction nextAction = idleActions[actionIndex];
                if (nextAction == m_lastAction) {
                    nextAction = idleActions[(actionIndex + 1) % actionCount];
                }
                m_action = nextAction;
                m_lastAction = nextAction;
                m_actionTimer = 0.0f;
                m_actionDir = (std::rand() % 2 == 0) ? -1.0f : 1.0f;
                m_shakeOffset = 1.8f * m_actionDir;
            }
        }
    }
}

void Pet::draw(D2DRenderer* renderer) {
    ID2D1RenderTarget* rt = renderer->getTarget();
    ID2D1Factory* factory = renderer->getFactory();
    if (!rt || !factory) return;

    int winW = renderer->getWidth();
    int winH = renderer->getHeight();
    float baseCx = (float)winW / 2.0f;
    float cx = baseCx + m_swayOffset;
    float baseCy = (float)winH / 2.0f + 8.0f;
    float cy = baseCy + m_bobOffset + m_jumpOffset;

    float s = (float)m_size / 120.0f;
    float sx = 1.0f + m_breathPhase * 0.03f;
    float sy = 1.0f - m_breathPhase * 0.03f;
    float rx = 56.0f * s * sx;
    float ry = 48.0f * s * sy;

    switch (m_style) {
    case PetStyle::Nailong:
        drawCharacterPet(renderer, cx, cy, s, PetStyle::Nailong);
        break;
    case PetStyle::Aimee:
        drawCharacterPet(renderer, cx, cy, s, PetStyle::Aimee);
        break;
    default:
        drawDefault(renderer, cx, cy, s, rx, ry);
        break;
    }
}

void Pet::setColor(float r, float g, float b) {
    m_r = r;
    m_g = g;
    m_b = b;
    if (m_style != PetStyle::Default) m_style = PetStyle::Default;
}

void Pet::setStyle(PetStyle style) {
    m_style = style;
    switch (style) {
    case PetStyle::Nailong:
        m_r = 1.0f; m_g = 0.80f; m_b = 0.22f;
        break;
    case PetStyle::Aimee:
        m_r = 0.96f; m_g = 0.74f; m_b = 0.84f;
        break;
    default:
        break;
    }
}

void Pet::drawDefault(D2DRenderer* renderer, float cx, float cy, float s, float rx, float ry) {
    ID2D1RenderTarget* rt = renderer->getTarget();
    ID2D1Factory* factory = renderer->getFactory();

    ID2D1SolidColorBrush* bodyBrush = renderer->getBrush(m_r, m_g, m_b);
    ID2D1SolidColorBrush* innerBrush = renderer->getBrush(
        std::min(m_r * 1.12f, 1.0f),
        std::min(m_g * 1.08f, 1.0f),
        std::min(m_b * 1.08f, 1.0f));

    drawEars(rt, factory, bodyBrush, innerBrush, cx, cy, s);
    drawFeet(rt, bodyBrush, cx, cy, rx, ry, s);
    drawBody(rt, bodyBrush, innerBrush, cx, cy, rx, ry);

    ID2D1SolidColorBrush* blushBrush = renderer->getBrush(1.0f, 0.45f, 0.50f, 0.38f);
    ID2D1SolidColorBrush* whiteBrush = renderer->getBrush(1.0f, 1.0f, 1.0f);
    ID2D1SolidColorBrush* darkBrush = renderer->getBrush(0.22f, 0.22f, 0.22f);

    drawBlush(rt, blushBrush, cx, cy, s);
    drawEyes(rt, whiteBrush, darkBrush, cx, cy, s);
    drawMouth(rt, factory, darkBrush, cx, cy, s);
}

void Pet::drawCapsule(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                      float x1, float y1, float x2, float y2, float thickness) {
    if (!rt || !brush) return;

    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0.01f || thickness <= 0.01f) return;

    float angle = std::atan2(dy, dx) * 180.0f / 3.14159265f;
    float cx = (x1 + x2) * 0.5f;
    float cy = (y1 + y2) * 0.5f;

    D2D1_MATRIX_3X2_F previous;
    rt->GetTransform(&previous);
    rt->SetTransform(D2D1::Matrix3x2F::Rotation(angle, D2D1::Point2F(cx, cy)) * previous);

    D2D1_ROUNDED_RECT rr = {
        D2D1::RectF(cx - len * 0.5f, cy - thickness * 0.5f,
                    cx + len * 0.5f, cy + thickness * 0.5f),
        thickness * 0.5f, thickness * 0.5f
    };
    rt->FillRoundedRectangle(rr, brush);

    rt->SetTransform(previous);
}

void Pet::drawCircle(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                     float cx, float cy, float radius) {
    if (!rt || !brush || radius <= 0.0f) return;
    D2D1_ELLIPSE ellipse = {{cx, cy}, radius, radius};
    rt->FillEllipse(ellipse, brush);
}

void Pet::drawCharacterPet(D2DRenderer* renderer, float cx, float cy, float s, PetStyle style) {
    float actionDuration = 1.0f;
    switch (m_action) {
    case SpriteAction::Wave:  actionDuration = 1.25f; break;
    case SpriteAction::Clap:  actionDuration = 1.00f; break;
    case SpriteAction::Hop:   actionDuration = 0.95f; break;
    case SpriteAction::Spin:  actionDuration = 1.20f; break;
    case SpriteAction::Nod:   actionDuration = 0.85f; break;
    case SpriteAction::Cheer: actionDuration = 1.10f; break;
    default:                  actionDuration = 1.0f;  break;
    }

    float progress = actionDuration > 0.0f ? clamp01(m_actionTimer / actionDuration) : 0.0f;
    float envelope = std::sin(progress * 3.14159265f);
    float flutter = std::sin(progress * TWO_PI * 3.0f);

    if (style == PetStyle::Aimee) {
        if (m_imgAimee) {
            drawSpriteCharacter(renderer, m_imgAimee, m_imgAimeeShadow,
                                cx, cy, s, style, progress, envelope, flutter);
        } else {
            drawAimeeCharacter(renderer, cx, cy, s, progress, envelope, flutter);
        }
    } else {
        if (m_imgNailong) {
            drawSpriteCharacter(renderer, m_imgNailong, m_imgNailongShadow,
                                cx, cy, s, style, progress, envelope, flutter);
        } else {
            drawNailongCharacter(renderer, cx, cy, s, progress, envelope, flutter);
        }
    }

    drawSpriteEffects(renderer, style, cx, cy, s);
}

void Pet::drawSpriteCharacter(D2DRenderer* renderer, ID2D1Bitmap* bitmap,
                              ID2D1Bitmap* shadowBitmap,
                              float cx, float cy, float s, PetStyle style,
                              float actionProgress, float envelope, float flutter) {
    ID2D1RenderTarget* rt = renderer->getTarget();
    if (!rt || !bitmap) return;

    D2D1_SIZE_F bmpSize = bitmap->GetSize();
    if (bmpSize.width <= 0.0f || bmpSize.height <= 0.0f) return;

    float scaleToHeight = style == PetStyle::Aimee ? 1.42f : 1.58f;
    float baseScale = (m_size * scaleToHeight) / bmpSize.height;
    float baseW = bmpSize.width * baseScale;
    float baseH = bmpSize.height * baseScale;

    float walkWave = m_isWalking ? std::sin(m_animTime * 10.5f) : 0.0f;
    float hopWave = m_isWalking ? std::cos(m_animTime * 6.0f) : 0.0f;
    float phase = std::sin(actionProgress * TWO_PI);
    float quickPhase = std::sin(actionProgress * TWO_PI * 2.0f);

    float dx = m_isWalking ? walkWave * 3.0f * s : 0.0f;
    float dy = 0.0f;
    float rotation = m_swayOffset * 0.10f + (m_isWalking ? walkWave * 2.8f : 0.0f);
    float scaleX = 1.0f + m_breathPhase * 0.018f;
    float scaleY = 1.0f - m_breathPhase * 0.014f;

    switch (m_action) {
    case SpriteAction::Wave:
        rotation += m_actionDir * (4.0f * phase + 2.0f * flutter) * envelope;
        dx += m_actionDir * 2.0f * s * envelope;
        break;
    case SpriteAction::Clap:
        scaleX -= 0.035f * envelope;
        scaleY += 0.030f * envelope;
        dy -= 2.0f * s * std::abs(quickPhase);
        break;
    case SpriteAction::Hop:
        dy -= 13.0f * s * envelope;
        scaleX += 0.055f * envelope;
        scaleY -= 0.045f * envelope;
        rotation += m_actionDir * 3.0f * phase;
        break;
    case SpriteAction::Spin:
        rotation += m_actionDir * (360.0f * actionProgress);
        scaleX += 0.018f * envelope;
        scaleY -= 0.014f * envelope;
        break;
    case SpriteAction::Nod:
        rotation += m_actionDir * 4.8f * quickPhase;
        dy += 2.5f * s * std::abs(quickPhase);
        scaleY -= 0.018f * envelope;
        break;
    case SpriteAction::Cheer:
        dy -= 8.0f * s * envelope;
        scaleX += 0.035f * envelope;
        scaleY += 0.025f * envelope;
        rotation += m_actionDir * (3.0f * phase + 1.4f * flutter) * envelope;
        break;
    default:
        break;
    }

    scaleX += m_isWalking ? hopWave * 0.010f : 0.0f;
    scaleY -= m_isWalking ? hopWave * 0.012f : 0.0f;

    float drawCx = cx + dx;
    float drawCy = cy + dy;
    D2D1_RECT_F dest = {
        drawCx - baseW * 0.5f,
        drawCy - baseH * 0.5f,
        drawCx + baseW * 0.5f,
        drawCy + baseH * 0.5f
    };

    float shadowAlpha = clamp01((style == PetStyle::Aimee ? 0.20f : 0.18f)
        - (m_action == SpriteAction::Hop ? 0.06f * envelope : 0.0f));
    ID2D1SolidColorBrush* groundShadow = renderer->getBrush(
        style == PetStyle::Aimee ? 0.14f : 0.16f,
        style == PetStyle::Aimee ? 0.10f : 0.12f,
        style == PetStyle::Aimee ? 0.13f : 0.06f,
        shadowAlpha);
    if (groundShadow) {
        D2D1_ELLIPSE shadow = {
            {cx, cy + baseH * 0.47f + 5.0f * s},
            std::max(24.0f * s, baseW * 0.33f),
            std::max(6.0f * s, baseH * 0.045f)
        };
        rt->FillEllipse(shadow, groundShadow);
    }

    if (shadowBitmap) {
        float shadowOpacity = style == PetStyle::Aimee ? 0.30f : 0.24f;
        shadowOpacity *= (m_action == SpriteAction::Hop) ? (1.0f - 0.25f * envelope) : 1.0f;
        D2D1_RECT_F shadowDest = {
            dest.left + baseW * 0.030f,
            dest.top + baseH * 0.045f + (m_action == SpriteAction::Hop ? baseH * 0.030f * envelope : 0.0f),
            dest.right + baseW * 0.030f,
            dest.bottom + baseH * 0.045f + (m_action == SpriteAction::Hop ? baseH * 0.030f * envelope : 0.0f)
        };
        rt->DrawBitmap(shadowBitmap, shadowDest, clamp01(shadowOpacity),
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
    }

    D2D1_MATRIX_3X2_F previous;
    rt->GetTransform(&previous);
    D2D1_POINT_2F pivot = D2D1::Point2F(drawCx, drawCy + baseH * 0.08f);
    D2D1_MATRIX_3X2_F transform =
        D2D1::Matrix3x2F::Scale(scaleX, scaleY, pivot) *
        D2D1::Matrix3x2F::Rotation(rotation, pivot) *
        previous;
    rt->SetTransform(transform);
    rt->DrawBitmap(bitmap, dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
    rt->SetTransform(previous);

    ID2D1SolidColorBrush* actionBrush = renderer->getBrush(
        style == PetStyle::Aimee ? 0.72f : 1.0f,
        style == PetStyle::Aimee ? 0.94f : 0.86f,
        style == PetStyle::Aimee ? 1.0f : 0.35f,
        0.62f * envelope);

    if (actionBrush && (m_action == SpriteAction::Wave || m_action == SpriteAction::Cheer)) {
        float side = m_actionDir >= 0.0f ? 1.0f : -1.0f;
        float x = drawCx + side * baseW * 0.46f;
        float y = drawCy - baseH * 0.22f;
        rt->DrawLine(D2D1::Point2F(x, y),
                     D2D1::Point2F(x + side * 10.0f * s, y - 7.0f * s),
                     actionBrush, 2.0f * s);
        rt->DrawLine(D2D1::Point2F(x + side * 3.0f * s, y + 9.0f * s),
                     D2D1::Point2F(x + side * 14.0f * s, y + 5.0f * s),
                     actionBrush, 1.8f * s);
    } else if (actionBrush && m_action == SpriteAction::Clap) {
        drawSparkle(rt, actionBrush, drawCx - 8.0f * s, drawCy - baseH * 0.10f, 3.6f * s, envelope);
        drawSparkle(rt, actionBrush, drawCx + 9.0f * s, drawCy - baseH * 0.09f, 3.2f * s, 1.0f - envelope * 0.2f);
    }
}

void Pet::drawAimeeCharacter(D2DRenderer* renderer, float cx, float cy, float s,
                             float actionProgress, float envelope, float flutter) {
    ID2D1RenderTarget* rt = renderer->getTarget();
    if (!rt) return;

    bool spinning = m_action == SpriteAction::Spin;
    float spinAngle = spinning ? (360.0f * actionProgress * m_actionDir) : 0.0f;

    D2D1_MATRIX_3X2_F previous;
    if (spinning) {
        rt->GetTransform(&previous);
        rt->SetTransform(D2D1::Matrix3x2F::Rotation(spinAngle, D2D1::Point2F(cx, cy)) * previous);
    }

    ID2D1SolidColorBrush* bodyBrush = renderer->getBrush(m_r, m_g, m_b);
    ID2D1SolidColorBrush* bodyShade = renderer->getBrush(
        std::max(m_r * 0.80f, 0.0f),
        std::max(m_g * 0.80f, 0.0f),
        std::max(m_b * 0.84f, 0.0f),
        0.96f);
    ID2D1SolidColorBrush* bodyLight = renderer->getBrush(
        std::min(m_r * 1.12f, 1.0f),
        std::min(m_g * 1.10f, 1.0f),
        std::min(m_b * 1.10f, 1.0f),
        0.85f);
    ID2D1SolidColorBrush* faceBrush = renderer->getBrush(1.0f, 0.97f, 0.98f);
    ID2D1SolidColorBrush* accentBrush = renderer->getBrush(0.75f, 0.63f, 0.98f);
    ID2D1SolidColorBrush* ribbonBrush = renderer->getBrush(0.95f, 0.58f, 0.76f);
    ID2D1SolidColorBrush* shadowBrush = renderer->getBrush(0.18f, 0.12f, 0.14f, 0.18f);
    ID2D1SolidColorBrush* whiteBrush = renderer->getBrush(1.0f, 1.0f, 1.0f);
    ID2D1SolidColorBrush* darkBrush = renderer->getBrush(0.25f, 0.20f, 0.30f);
    ID2D1SolidColorBrush* blushBrush = renderer->getBrush(1.0f, 0.65f, 0.74f, 0.30f);

    float hopLift = (m_action == SpriteAction::Hop) ? envelope * 9.0f * s : 0.0f;
    float cheerLift = (m_action == SpriteAction::Cheer) ? envelope * 4.5f * s : 0.0f;
    float bodyCy = cy + 10.0f * s - hopLift * 0.55f - cheerLift * 0.30f;
    float headCy = cy - 35.0f * s - hopLift * 0.40f - cheerLift * 0.25f;
    float nodWobble = (m_action == SpriteAction::Nod) ? std::sin(actionProgress * TWO_PI * 2.0f) * 2.3f * s : 0.0f;
    headCy += nodWobble;

    float shadowAlpha = 0.20f - (m_action == SpriteAction::Hop ? 0.07f * envelope : 0.0f);
    ID2D1SolidColorBrush* groundShadow = renderer->getBrush(0.18f, 0.11f, 0.12f, clamp01(shadowAlpha));
    D2D1_ELLIPSE shadow = {{cx, cy + 52.0f * s}, 30.0f * s, 7.0f * s};
    rt->FillEllipse(shadow, groundShadow);
    D2D1_ELLIPSE backShadow = {{cx, bodyCy + 4.0f * s}, 16.0f * s, 24.0f * s};
    rt->FillEllipse(backShadow, shadowBrush);

    // Back frills and hood.
    D2D1_ELLIPSE hood = {{cx, headCy + 2.0f * s}, 31.0f * s, 28.0f * s};
    rt->FillEllipse(hood, bodyShade);
    D2D1_ELLIPSE head = {{cx, headCy}, 24.0f * s, 21.0f * s};
    rt->FillEllipse(head, faceBrush);
    D2D1_ELLIPSE headLight = {{cx - 6.0f * s, headCy - 4.0f * s}, 15.0f * s, 13.0f * s};
    rt->FillEllipse(headLight, bodyLight);

    // Body and dress.
    D2D1_ELLIPSE body = {{cx, bodyCy}, 24.0f * s, 30.0f * s};
    rt->FillEllipse(body, bodyBrush);
    D2D1_ELLIPSE bodyInner = {{cx - 2.5f * s, bodyCy + 4.0f * s}, 18.0f * s, 22.0f * s};
    rt->FillEllipse(bodyInner, bodyLight);

    // Side frills / wings.
    float wingBend = 10.0f * s + envelope * 4.0f * s + flutter * 1.2f * s;
    drawCapsule(rt, accentBrush, cx - 26.0f * s, bodyCy - 1.0f * s,
                cx - 40.0f * s, bodyCy + 8.0f * s + wingBend * 0.10f, 9.0f * s);
    drawCapsule(rt, accentBrush, cx + 26.0f * s, bodyCy - 1.0f * s,
                cx + 40.0f * s, bodyCy + 8.0f * s - wingBend * 0.10f, 9.0f * s);
    drawCapsule(rt, bodyLight, cx - 24.0f * s, bodyCy - 4.0f * s,
                cx - 34.0f * s, bodyCy + 3.0f * s + wingBend * 0.06f, 5.0f * s);
    drawCapsule(rt, bodyLight, cx + 24.0f * s, bodyCy - 4.0f * s,
                cx + 34.0f * s, bodyCy + 3.0f * s - wingBend * 0.06f, 5.0f * s);

    // Legs and feet.
    drawCapsule(rt, bodyShade, cx - 11.0f * s, bodyCy + 22.0f * s, cx - 13.0f * s, bodyCy + 39.0f * s - hopLift * 0.15f, 8.0f * s);
    drawCapsule(rt, bodyShade, cx + 11.0f * s, bodyCy + 22.0f * s, cx + 13.0f * s, bodyCy + 39.0f * s - hopLift * 0.15f, 8.0f * s);
    drawCircle(rt, bodyShade, cx - 15.0f * s, bodyCy + 42.0f * s - hopLift * 0.15f, 5.4f * s);
    drawCircle(rt, bodyShade, cx + 15.0f * s, bodyCy + 42.0f * s - hopLift * 0.15f, 5.4f * s);

    // Ribbon / bow.
    drawCircle(rt, ribbonBrush, cx, headCy - 23.0f * s, 3.4f * s);
    drawCircle(rt, ribbonBrush, cx - 7.0f * s, headCy - 25.0f * s, 4.0f * s);
    drawCircle(rt, ribbonBrush, cx + 7.0f * s, headCy - 25.0f * s, 4.0f * s);

    // Arms.
    float waveSide = (m_actionDir >= 0.0f) ? 1.0f : -1.0f;
    float leftShoulderX = cx - 19.0f * s;
    float rightShoulderX = cx + 19.0f * s;
    float shoulderY = bodyCy - 8.0f * s;
    float frontLift = (m_action == SpriteAction::Wave || m_action == SpriteAction::Cheer || m_action == SpriteAction::Clap) ? envelope : 0.0f;

    float leftHandX = leftShoulderX - 12.0f * s;
    float leftHandY = shoulderY + 12.0f * s;
    float rightHandX = rightShoulderX + 12.0f * s;
    float rightHandY = shoulderY + 12.0f * s;

    if (m_action == SpriteAction::Wave) {
        if (waveSide > 0.0f) {
            rightHandX = rightShoulderX + 22.0f * s;
            rightHandY = shoulderY - 20.0f * s - 6.0f * s * frontLift;
            leftHandX = leftShoulderX - 8.0f * s;
            leftHandY = shoulderY + 14.0f * s;
        } else {
            leftHandX = leftShoulderX - 22.0f * s;
            leftHandY = shoulderY - 20.0f * s - 6.0f * s * frontLift;
            rightHandX = rightShoulderX + 8.0f * s;
            rightHandY = shoulderY + 14.0f * s;
        }
    } else if (m_action == SpriteAction::Clap) {
        leftHandX = cx - 4.5f * s;
        leftHandY = bodyCy - 4.0f * s - 3.0f * s * frontLift;
        rightHandX = cx + 4.5f * s;
        rightHandY = bodyCy - 4.0f * s - 3.0f * s * frontLift;
    } else if (m_action == SpriteAction::Cheer) {
        leftHandX = leftShoulderX - 18.0f * s;
        leftHandY = shoulderY - 20.0f * s;
        rightHandX = rightShoulderX + 18.0f * s;
        rightHandY = shoulderY - 20.0f * s;
    } else if (m_action == SpriteAction::Hop) {
        leftHandX = leftShoulderX - 10.0f * s;
        leftHandY = shoulderY + 8.0f * s - 4.0f * s * envelope;
        rightHandX = rightShoulderX + 10.0f * s;
        rightHandY = shoulderY + 8.0f * s - 4.0f * s * envelope;
    } else if (m_action == SpriteAction::Nod) {
        leftHandY = shoulderY + 11.0f * s;
        rightHandY = shoulderY + 11.0f * s;
    }

    drawCapsule(rt, bodyShade, leftShoulderX, shoulderY, leftHandX, leftHandY, 8.5f * s);
    drawCapsule(rt, bodyShade, rightShoulderX, shoulderY, rightHandX, rightHandY, 8.5f * s);
    drawCircle(rt, faceBrush, leftHandX, leftHandY, 4.1f * s);
    drawCircle(rt, faceBrush, rightHandX, rightHandY, 4.1f * s);

    // Facial details.
    drawEyes(rt, whiteBrush, darkBrush, cx, headCy + 1.0f * s, s * 0.82f);
    drawMouth(rt, renderer->getFactory(), darkBrush, cx, headCy + 1.0f * s, s * 0.68f);
    drawBlush(rt, blushBrush, cx, headCy + 1.0f * s, s * 0.78f);

    if (spinning) {
        rt->SetTransform(previous);
    }
}

void Pet::drawNailongCharacter(D2DRenderer* renderer, float cx, float cy, float s,
                               float actionProgress, float envelope, float flutter) {
    ID2D1RenderTarget* rt = renderer->getTarget();
    if (!rt) return;

    bool spinning = m_action == SpriteAction::Spin;
    float spinAngle = spinning ? (360.0f * actionProgress * m_actionDir) : 0.0f;

    D2D1_MATRIX_3X2_F previous;
    if (spinning) {
        rt->GetTransform(&previous);
        rt->SetTransform(D2D1::Matrix3x2F::Rotation(spinAngle, D2D1::Point2F(cx, cy)) * previous);
    }

    ID2D1SolidColorBrush* bodyBrush = renderer->getBrush(m_r, m_g, m_b);
    ID2D1SolidColorBrush* bodyShade = renderer->getBrush(
        std::max(m_r * 0.82f, 0.0f),
        std::max(m_g * 0.78f, 0.0f),
        std::max(m_b * 0.72f, 0.0f),
        0.95f);
    ID2D1SolidColorBrush* bellyBrush = renderer->getBrush(1.0f, 0.97f, 0.82f);
    ID2D1SolidColorBrush* accentBrush = renderer->getBrush(0.96f, 0.60f, 0.15f);
    ID2D1SolidColorBrush* hornBrush = renderer->getBrush(0.94f, 0.70f, 0.22f);
    ID2D1SolidColorBrush* highlightBrush = renderer->getBrush(1.0f, 1.0f, 1.0f, 0.20f);
    ID2D1SolidColorBrush* shadowBrush = renderer->getBrush(0.18f, 0.11f, 0.04f, 0.18f);
    ID2D1SolidColorBrush* whiteBrush = renderer->getBrush(1.0f, 1.0f, 1.0f);
    ID2D1SolidColorBrush* darkBrush = renderer->getBrush(0.28f, 0.20f, 0.10f);

    float hopLift = (m_action == SpriteAction::Hop) ? envelope * 10.0f * s : 0.0f;
    float cheerLift = (m_action == SpriteAction::Cheer) ? envelope * 4.0f * s : 0.0f;
    float bodyCy = cy + 8.0f * s - hopLift * 0.55f - cheerLift * 0.25f;
    float headCy = cy - 32.0f * s - hopLift * 0.35f - cheerLift * 0.18f;
    float nodWobble = (m_action == SpriteAction::Nod) ? std::sin(actionProgress * TWO_PI * 2.0f) * 2.0f * s : 0.0f;
    headCy += nodWobble;

    float tailWiggle = std::sin(actionProgress * TWO_PI * 2.5f + m_actionDir) * 4.0f * s * envelope
                     + flutter * 1.2f * s;
    float shadowAlpha = 0.20f - (m_action == SpriteAction::Hop ? 0.08f * envelope : 0.0f);
    ID2D1SolidColorBrush* groundShadow = renderer->getBrush(0.18f, 0.11f, 0.04f, clamp01(shadowAlpha));
    D2D1_ELLIPSE shadow = {{cx, cy + 54.0f * s}, 34.0f * s, 8.0f * s};
    rt->FillEllipse(shadow, groundShadow);

    // Tail first.
    float tailSide = m_actionDir >= 0.0f ? 1.0f : -1.0f;
    drawCapsule(rt, accentBrush,
                cx + 24.0f * s, bodyCy + 10.0f * s,
                cx + 43.0f * s * tailSide, bodyCy + 6.0f * s + tailWiggle,
                8.0f * s);
    drawCapsule(rt, accentBrush,
                cx + 43.0f * s * tailSide, bodyCy + 6.0f * s + tailWiggle,
                cx + 55.0f * s * tailSide, bodyCy + 10.0f * s + tailWiggle * 0.55f,
                6.5f * s);
    drawCircle(rt, accentBrush, cx + 58.0f * s * tailSide, bodyCy + 10.0f * s + tailWiggle * 0.55f, 3.6f * s);

    // Body and belly.
    D2D1_ELLIPSE body = {{cx, bodyCy}, 28.0f * s, 32.0f * s};
    rt->FillEllipse(body, bodyBrush);
    D2D1_ELLIPSE bodyInner = {{cx - 2.0f * s, bodyCy + 4.0f * s}, 19.0f * s, 22.0f * s};
    rt->FillEllipse(bodyInner, bellyBrush);
    D2D1_ELLIPSE bodyLight = {{cx - 8.0f * s, bodyCy - 6.0f * s}, 13.0f * s, 14.0f * s};
    rt->FillEllipse(bodyLight, highlightBrush);

    // Back spikes.
    drawCapsule(rt, hornBrush, cx - 18.0f * s, bodyCy - 18.0f * s, cx - 24.0f * s, bodyCy - 29.0f * s, 4.0f * s);
    drawCapsule(rt, hornBrush, cx - 8.0f * s, bodyCy - 20.0f * s, cx - 12.0f * s, bodyCy - 31.0f * s, 4.0f * s);
    drawCapsule(rt, hornBrush, cx + 2.0f * s, bodyCy - 20.0f * s, cx + 4.0f * s, bodyCy - 31.0f * s, 4.0f * s);

    // Head and snout.
    D2D1_ELLIPSE head = {{cx, headCy}, 27.0f * s, 24.0f * s};
    rt->FillEllipse(head, bodyBrush);
    D2D1_ELLIPSE snout = {{cx + 5.0f * s, headCy + 6.0f * s}, 15.0f * s, 10.0f * s};
    rt->FillEllipse(snout, bellyBrush);
    D2D1_ELLIPSE headLight = {{cx - 7.0f * s, headCy - 5.0f * s}, 13.0f * s, 11.0f * s};
    rt->FillEllipse(headLight, highlightBrush);

    // Horns and tiny ears.
    drawCapsule(rt, hornBrush, cx - 11.0f * s, headCy - 18.0f * s, cx - 17.0f * s, headCy - 31.0f * s, 4.2f * s);
    drawCapsule(rt, hornBrush, cx + 11.0f * s, headCy - 18.0f * s, cx + 17.0f * s, headCy - 31.0f * s, 4.2f * s);

    // Arms.
    float waveSide = (m_actionDir >= 0.0f) ? 1.0f : -1.0f;
    float leftShoulderX = cx - 18.0f * s;
    float rightShoulderX = cx + 18.0f * s;
    float shoulderY = bodyCy - 4.0f * s;

    float leftHandX = leftShoulderX - 10.0f * s;
    float leftHandY = shoulderY + 10.0f * s;
    float rightHandX = rightShoulderX + 10.0f * s;
    float rightHandY = shoulderY + 10.0f * s;

    if (m_action == SpriteAction::Wave) {
        if (waveSide > 0.0f) {
            rightHandX = rightShoulderX + 20.0f * s;
            rightHandY = shoulderY - 18.0f * s - 5.0f * s * envelope;
            leftHandX = leftShoulderX - 8.0f * s;
            leftHandY = shoulderY + 12.0f * s;
        } else {
            leftHandX = leftShoulderX - 20.0f * s;
            leftHandY = shoulderY - 18.0f * s - 5.0f * s * envelope;
            rightHandX = rightShoulderX + 8.0f * s;
            rightHandY = shoulderY + 12.0f * s;
        }
    } else if (m_action == SpriteAction::Clap) {
        leftHandX = cx - 4.0f * s;
        leftHandY = bodyCy - 2.0f * s - 3.0f * s * envelope;
        rightHandX = cx + 4.0f * s;
        rightHandY = bodyCy - 2.0f * s - 3.0f * s * envelope;
    } else if (m_action == SpriteAction::Cheer) {
        leftHandX = leftShoulderX - 18.0f * s;
        leftHandY = shoulderY - 18.0f * s;
        rightHandX = rightShoulderX + 18.0f * s;
        rightHandY = shoulderY - 18.0f * s;
    } else if (m_action == SpriteAction::Hop) {
        leftHandX = leftShoulderX - 10.0f * s;
        leftHandY = shoulderY + 7.0f * s - 4.0f * s * envelope;
        rightHandX = rightShoulderX + 10.0f * s;
        rightHandY = shoulderY + 7.0f * s - 4.0f * s * envelope;
    } else if (m_action == SpriteAction::Nod) {
        leftHandY = shoulderY + 8.0f * s;
        rightHandY = shoulderY + 8.0f * s;
    }

    drawCapsule(rt, shadowBrush, leftShoulderX, shoulderY, leftHandX, leftHandY, 7.5f * s);
    drawCapsule(rt, shadowBrush, rightShoulderX, shoulderY, rightHandX, rightHandY, 7.5f * s);
    drawCircle(rt, bodyShade, leftHandX, leftHandY, 3.8f * s);
    drawCircle(rt, bodyShade, rightHandX, rightHandY, 3.8f * s);

    // Legs.
    drawCapsule(rt, bodyShade, cx - 10.0f * s, bodyCy + 24.0f * s, cx - 12.0f * s, bodyCy + 39.0f * s - hopLift * 0.2f, 7.0f * s);
    drawCapsule(rt, bodyShade, cx + 10.0f * s, bodyCy + 24.0f * s, cx + 12.0f * s, bodyCy + 39.0f * s - hopLift * 0.2f, 7.0f * s);
    drawCircle(rt, bodyBrush, cx - 14.0f * s, bodyCy + 43.0f * s - hopLift * 0.2f, 4.9f * s);
    drawCircle(rt, bodyBrush, cx + 14.0f * s, bodyCy + 43.0f * s - hopLift * 0.2f, 4.9f * s);

    // Face.
    drawEyes(rt, whiteBrush, darkBrush, cx, headCy + 1.5f * s, s * 0.80f);
    drawMouth(rt, renderer->getFactory(), darkBrush, cx, headCy + 3.0f * s, s * 0.66f);
    drawBlush(rt, renderer->getBrush(1.0f, 0.66f, 0.43f, 0.22f), cx, headCy + 3.0f * s, s * 0.60f);

    // Nostrils.
    drawCircle(rt, darkBrush, cx - 3.0f * s, headCy + 7.0f * s, 1.0f * s);
    drawCircle(rt, darkBrush, cx + 3.0f * s, headCy + 7.0f * s, 1.0f * s);

    if (spinning) {
        rt->SetTransform(previous);
    }
}

void Pet::drawSpriteEffects(D2DRenderer* renderer, PetStyle style,
                            float cx, float cy, float s) {
    ID2D1RenderTarget* rt = renderer->getTarget();
    if (!rt) return;

    ID2D1SolidColorBrush* happyBrush = nullptr;
    ID2D1SolidColorBrush* thoughtBrush = nullptr;
    ID2D1SolidColorBrush* sleepyBrush = nullptr;
    ID2D1SolidColorBrush* surpriseBrush = nullptr;

    if (style == PetStyle::Aimee) {
        happyBrush = renderer->getBrush(0.98f, 0.74f, 0.96f, 0.86f);
        thoughtBrush = renderer->getBrush(0.72f, 0.92f, 1.00f, 0.78f);
        sleepyBrush = renderer->getBrush(0.92f, 0.95f, 1.00f, 0.74f);
        surpriseBrush = renderer->getBrush(1.00f, 1.00f, 1.00f, 0.90f);
    } else {
        happyBrush = renderer->getBrush(1.00f, 0.92f, 0.48f, 0.86f);
        thoughtBrush = renderer->getBrush(1.00f, 0.98f, 0.80f, 0.78f);
        sleepyBrush = renderer->getBrush(0.96f, 0.96f, 0.90f, 0.72f);
        surpriseBrush = renderer->getBrush(1.00f, 1.00f, 1.00f, 0.90f);
    }

    if (m_mood == PetMood::Happy) {
        float beat = 0.5f + 0.5f * std::sin(m_animTime * 4.0f);
        drawSparkle(rt, happyBrush, cx - 34.0f * s, cy - 86.0f * s - beat * 3.0f * s, 5.0f * s, beat);
        drawSparkle(rt, happyBrush, cx + 42.0f * s, cy - 78.0f * s + beat * 2.0f * s, 4.4f * s, 1.0f - beat * 0.2f);
    } else if (m_mood == PetMood::Curious || m_mood == PetMood::Thinking) {
        drawThoughtDots(rt, thoughtBrush, cx + 24.0f * s, cy - 88.0f * s, s, 0.85f);
    } else if (m_mood == PetMood::Sleepy) {
        float wobble = std::sin(m_animTime * 2.0f) * 1.5f * s;
        float x = cx + 16.0f * s;
        float y = cy - 92.0f * s + wobble;
        rt->DrawLine(D2D1::Point2F(x, y), D2D1::Point2F(x + 6.0f * s, y - 8.0f * s), sleepyBrush, 2.0f);
        rt->DrawLine(D2D1::Point2F(x + 6.0f * s, y - 8.0f * s), D2D1::Point2F(x, y - 16.0f * s), sleepyBrush, 2.0f);
        rt->DrawLine(D2D1::Point2F(x + 10.0f * s, y - 20.0f * s), D2D1::Point2F(x + 16.0f * s, y - 28.0f * s), sleepyBrush, 2.0f);
    } else if (m_mood == PetMood::Surprised) {
        rt->DrawLine(D2D1::Point2F(cx, cy - 96.0f * s), D2D1::Point2F(cx, cy - 114.0f * s), surpriseBrush, 2.3f);
        D2D1_ELLIPSE dot = {{cx, cy - 88.0f * s}, 2.2f * s, 2.2f * s};
        rt->FillEllipse(dot, surpriseBrush);
    }

    if (m_isWalking) {
        float step = 0.5f + 0.5f * std::sin(m_animTime * 10.5f);
        drawThoughtDots(rt, thoughtBrush, cx + 5.0f * s, cy + 86.0f * s, s * 0.6f, 0.35f + 0.25f * step);
    }
}

void Pet::drawSparkle(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                      float x, float y, float radius, float alpha) {
    if (!rt || !brush) return;
    float arm = radius * (0.75f + 0.3f * alpha);
    float thickness = std::max(1.2f, radius * 0.14f);
    rt->DrawLine(D2D1::Point2F(x - arm, y), D2D1::Point2F(x + arm, y), brush, thickness);
    rt->DrawLine(D2D1::Point2F(x, y - arm), D2D1::Point2F(x, y + arm), brush, thickness);
    rt->DrawLine(D2D1::Point2F(x - arm * 0.7f, y - arm * 0.7f), D2D1::Point2F(x + arm * 0.7f, y + arm * 0.7f), brush, thickness * 0.8f);
    rt->DrawLine(D2D1::Point2F(x - arm * 0.7f, y + arm * 0.7f), D2D1::Point2F(x + arm * 0.7f, y - arm * 0.7f), brush, thickness * 0.8f);
    D2D1_ELLIPSE center = {{x, y}, radius * 0.18f, radius * 0.18f};
    rt->FillEllipse(center, brush);
}

void Pet::drawThoughtDots(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                          float x, float y, float scale, float alpha) {
    if (!rt || !brush) return;
    float size = scale * (1.6f + alpha);
    float drift = std::sin(m_animTime * 2.0f) * scale * 1.2f;
    D2D1_ELLIPSE a = {{x, y + drift}, 2.0f * size, 2.0f * size};
    D2D1_ELLIPSE b = {{x + 8.0f * scale, y - 9.0f * scale + drift * 0.7f}, 3.0f * size, 3.0f * size};
    D2D1_ELLIPSE c = {{x + 17.0f * scale, y - 18.0f * scale + drift * 0.4f}, 4.4f * size, 4.4f * size};
    rt->FillEllipse(a, brush);
    rt->FillEllipse(b, brush);
    rt->FillEllipse(c, brush);
}

void Pet::setMood(PetMood mood) {
    m_mood = mood;
}

void Pet::startSpriteAction(SpriteAction action) {
    m_action = action;
    m_lastAction = action;
    m_actionTimer = 0.0f;
    m_actionDir = (std::rand() % 2 == 0) ? -1.0f : 1.0f;

    float jumpScale = 0.90f;
    float shakeAmount = 5.0f;
    switch (m_action) {
    case SpriteAction::Clap:
        jumpScale = 0.68f;
        shakeAmount = 3.6f;
        break;
    case SpriteAction::Hop:
        jumpScale = 1.22f;
        shakeAmount = 7.2f;
        break;
    case SpriteAction::Spin:
        jumpScale = 0.88f;
        shakeAmount = 5.8f;
        break;
    case SpriteAction::Nod:
        jumpScale = 0.42f;
        shakeAmount = 2.4f;
        break;
    case SpriteAction::Cheer:
        jumpScale = 1.04f;
        shakeAmount = 6.2f;
        break;
    case SpriteAction::Wave:
    default:
        jumpScale = 0.82f;
        shakeAmount = 5.2f;
        break;
    }

    SoundPlayer::playJump();
    m_isJumping = true;
    m_jumpVel = JUMP_INIT_VEL * jumpScale;
    m_jumpOffset = 0.0f;
    m_shakeOffset = shakeAmount * m_actionDir;
}

void Pet::playAction(PetAction action) {
    switch (action) {
    case PetAction::Wave:
        startSpriteAction(SpriteAction::Wave);
        break;
    case PetAction::Clap:
        startSpriteAction(SpriteAction::Clap);
        break;
    case PetAction::Hop:
        startSpriteAction(SpriteAction::Hop);
        break;
    case PetAction::Spin:
        startSpriteAction(SpriteAction::Spin);
        break;
    case PetAction::Nod:
        startSpriteAction(SpriteAction::Nod);
        break;
    case PetAction::Cheer:
        startSpriteAction(SpriteAction::Cheer);
        break;
    }
}

void Pet::onClick() {
    switch (m_mood) {
    case PetMood::Idle:
        setMood(PetMood::Happy);
        break;
    case PetMood::Happy:
        setMood(PetMood::Surprised);
        break;
    case PetMood::Surprised:
    default:
        setMood(PetMood::Idle);
        break;
    }

    static const SpriteAction actions[] = {
        SpriteAction::Wave,
        SpriteAction::Clap,
        SpriteAction::Hop,
        SpriteAction::Spin,
        SpriteAction::Nod,
        SpriteAction::Cheer
    };

    int actionCount = (int)(sizeof(actions) / sizeof(actions[0]));
    int actionIndex = std::rand() % actionCount;
    SpriteAction nextAction = actions[actionIndex];
    if (nextAction == m_lastAction) {
        nextAction = actions[(actionIndex + 1) % actionCount];
    }

    startSpriteAction(nextAction);
}

void Pet::drawBody(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                   ID2D1SolidColorBrush* innerBrush,
                   float cx, float cy, float rx, float ry) {
    D2D1_ELLIPSE body = {{cx, cy}, rx, ry};
    rt->FillEllipse(body, brush);

    float innerRx = rx * 0.72f;
    float innerRy = ry * 0.68f;
    float innerCy = cy + ry * 0.08f;
    D2D1_ELLIPSE inner = {{cx, innerCy}, innerRx, innerRy};
    rt->FillEllipse(inner, innerBrush);
}

void Pet::drawEars(ID2D1RenderTarget* rt, ID2D1Factory* factory,
                   ID2D1SolidColorBrush* brush, ID2D1SolidColorBrush* innerBrush,
                   float cx, float cy, float s) {
    float earW = 22.0f * s;
    float earH = 18.0f * s;
    float earY = cy - 42.0f * s;

    float lTipX = cx - 20.0f * s;
    float lTipY = earY - earH;
    float lBlX = cx - 33.0f * s;
    float lBlY = earY;
    float lBrX = cx - 10.0f * s;
    float lBrY = earY;

    float rTipX = cx + 20.0f * s;
    float rTipY = earY - earH;
    float rBlX = cx + 10.0f * s;
    float rBlY = earY;
    float rBrX = cx + 33.0f * s;
    float rBrY = earY;

    ID2D1PathGeometry* geo = nullptr;
    ID2D1GeometrySink* sink = nullptr;

    auto drawTri = [&](float x1, float y1, float x2, float y2, float x3, float y3) {
        factory->CreatePathGeometry(&geo);
        geo->Open(&sink);
        sink->BeginFigure(D2D1::Point2F(x1, y1), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine(D2D1::Point2F(x2, y2));
        sink->AddLine(D2D1::Point2F(x3, y3));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        sink->Close();
        sink->Release();
        rt->FillGeometry(geo, brush);
        geo->Release();
        geo = nullptr;
    };

    drawTri(lTipX, lTipY, lBlX, lBlY, lBrX, lBrY);
    drawTri(rTipX, rTipY, rBlX, rBlY, rBrX, rBrY);

    float inScale = 0.55f;
    auto drawInner = [&](float tipX, float tipY, float blX, float blY, float brX, float brY) {
        float cxE = (tipX + blX + brX) / 3.0f;
        float cyE = (tipY + blY + brY) / 3.0f;
        float iTipX = cxE + (tipX - cxE) * inScale;
        float iTipY = cyE + (tipY - cyE) * inScale;
        float iBlX = cxE + (blX - cxE) * inScale;
        float iBlY = cyE + (blY - cyE) * inScale;
        float iBrX = cxE + (brX - cxE) * inScale;
        float iBrY = cyE + (brY - cyE) * inScale;
        factory->CreatePathGeometry(&geo);
        geo->Open(&sink);
        sink->BeginFigure(D2D1::Point2F(iTipX, iTipY), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine(D2D1::Point2F(iBlX, iBlY));
        sink->AddLine(D2D1::Point2F(iBrX, iBrY));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        sink->Close();
        sink->Release();
        rt->FillGeometry(geo, innerBrush);
        geo->Release();
    };

    drawInner(lTipX, lTipY, lBlX, lBlY, lBrX, lBrY);
    drawInner(rTipX, rTipY, rBlX, rBlY, rBrX, rBrY);
}

void Pet::drawFeet(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                   float cx, float cy, float rx, float ry, float s) {
    float footY = cy + ry - 3.0f * s;
    float footRx = 9.0f * s;
    float footRy = 4.5f * s;
    float footOffX = 17.0f * s;

    D2D1_ELLIPSE lFoot = {{cx - footOffX, footY}, footRx, footRy};
    D2D1_ELLIPSE rFoot = {{cx + footOffX, footY}, footRx, footRy};
    rt->FillEllipse(lFoot, brush);
    rt->FillEllipse(rFoot, brush);
}

void Pet::drawEyes(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* white,
                   ID2D1SolidColorBrush* dark, float cx, float cy, float s) {
    float moodEye = 1.0f;
    switch (m_mood) {
    case PetMood::Happy:
        moodEye = 0.3f;
        break;
    case PetMood::Surprised:
        moodEye = 1.45f;
        break;
    default:
        moodEye = 1.0f;
        break;
    }

    float eyeOffX = 15.0f * s;
    float eyeOffY = -4.0f * s;
    float eyeRx = 9.5f * s;
    float eyeRy = 13.0f * s * m_eyeScaleY * moodEye;

    D2D1_ELLIPSE lEye = {{cx - eyeOffX, cy + eyeOffY}, eyeRx, eyeRy};
    D2D1_ELLIPSE rEye = {{cx + eyeOffX, cy + eyeOffY}, eyeRx, eyeRy};
    rt->FillEllipse(lEye, white);
    rt->FillEllipse(rEye, white);

    if (m_eyeScaleY < 0.05f) return;

    float pupilR = 5.5f * s * moodEye;
    float pupilOffY = eyeRy * 0.2f;
    D2D1_ELLIPSE lPupil = {{cx - eyeOffX, cy + eyeOffY + pupilOffY}, pupilR, pupilR};
    D2D1_ELLIPSE rPupil = {{cx + eyeOffX, cy + eyeOffY + pupilOffY}, pupilR, pupilR};
    rt->FillEllipse(lPupil, dark);
    rt->FillEllipse(rPupil, dark);

    float glintR = 2.2f * s * moodEye;
    float glintOffX = pupilR * 0.35f;
    float glintOffY = -pupilR * 0.3f;
    D2D1_ELLIPSE lGlint = {{cx - eyeOffX + glintOffX, cy + eyeOffY + pupilOffY + glintOffY}, glintR, glintR};
    D2D1_ELLIPSE rGlint = {{cx + eyeOffX + glintOffX, cy + eyeOffY + pupilOffY + glintOffY}, glintR, glintR};
    rt->FillEllipse(lGlint, white);
    rt->FillEllipse(rGlint, white);
}

void Pet::drawBlush(ID2D1RenderTarget* rt, ID2D1SolidColorBrush* brush,
                    float cx, float cy, float s) {
    float blushOffX = 26.0f * s;
    float blushOffY = 12.0f * s;
    float blushRx = 8.0f * s;
    float blushRy = 4.5f * s;

    D2D1_ELLIPSE lBlush = {{cx - blushOffX, cy + blushOffY}, blushRx, blushRy};
    D2D1_ELLIPSE rBlush = {{cx + blushOffX, cy + blushOffY}, blushRx, blushRy};
    rt->FillEllipse(lBlush, brush);
    rt->FillEllipse(rBlush, brush);
}

void Pet::drawMouth(ID2D1RenderTarget* rt, ID2D1Factory* factory,
                    ID2D1SolidColorBrush* brush, float cx, float cy, float s) {
    if (m_mood == PetMood::Surprised) {
        float oY = cy + 16.0f * s;
        float oR = 6.0f * s;
        D2D1_ELLIPSE oMouth = {{cx, oY}, oR, oR * 0.8f};
        rt->FillEllipse(oMouth, brush);
        return;
    }

    float mouthScale = (m_mood == PetMood::Happy) ? 1.6f : 1.0f;
    float mouthY = cy + 14.0f * s;
    float halfW = 9.0f * s * mouthScale;
    float arcH = 6.0f * s * mouthScale;

    ID2D1PathGeometry* geo = nullptr;
    ID2D1GeometrySink* sink = nullptr;
    factory->CreatePathGeometry(&geo);
    geo->Open(&sink);

    sink->BeginFigure(D2D1::Point2F(cx - halfW, mouthY), D2D1_FIGURE_BEGIN_HOLLOW);

    D2D1_ARC_SEGMENT arc = {};
    arc.point = D2D1::Point2F(cx + halfW, mouthY);
    arc.size = D2D1::SizeF(halfW, arcH);
    arc.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
    arc.arcSize = D2D1_ARC_SIZE_SMALL;
    sink->AddArc(arc);

    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    sink->Close();
    sink->Release();

    rt->DrawGeometry(geo, brush, 1.8f * s);
    geo->Release();
}
