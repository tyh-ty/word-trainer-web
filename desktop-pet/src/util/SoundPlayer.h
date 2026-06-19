#pragma once
#include <vector>
#include <cstdint>

class SoundPlayer {
public:
    static void playClick();
    static void playJump();
    static void playMoodChange();

private:
    static void playTone(float freq, float duration, float volume = 0.3f);
    static std::vector<uint8_t> makeWav(const std::vector<int16_t>& samples);
};
