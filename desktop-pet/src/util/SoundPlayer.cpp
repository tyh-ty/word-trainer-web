#include "SoundPlayer.h"
#include <windows.h>
#include <cmath>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

std::vector<uint8_t> SoundPlayer::makeWav(const std::vector<int16_t>& samples) {
    uint32_t dataSize = (uint32_t)(samples.size() * 2);
    uint32_t fileSize = 44 + dataSize;

    std::vector<uint8_t> wav(44 + dataSize);
    auto* p = wav.data();

    // RIFF header
    p[0]='R';p[1]='I';p[2]='F';p[3]='F';
    *(uint32_t*)(p+4)=fileSize-8;
    p[8]='W';p[9]='A';p[10]='V';p[11]='E';
    // fmt chunk
    p[12]='f';p[13]='m';p[14]='t';p[15]=' ';
    *(uint32_t*)(p+16)=16;          // chunk size
    *(uint16_t*)(p+20)=1;           // PCM
    *(uint16_t*)(p+22)=1;           // mono
    *(uint32_t*)(p+24)=44100;       // sample rate
    *(uint32_t*)(p+28)=88200;       // byte rate
    *(uint16_t*)(p+32)=2;           // block align
    *(uint16_t*)(p+34)=16;          // bits per sample
    // data chunk
    p[36]='d';p[37]='a';p[38]='t';p[39]='a';
    *(uint32_t*)(p+40)=dataSize;
    for (size_t i = 0; i < samples.size(); i++) {
        p[44+i*2] = (uint8_t)(samples[i] & 0xFF);
        p[44+i*2+1] = (uint8_t)((samples[i] >> 8) & 0xFF);
    }
    return wav;
}

void SoundPlayer::playTone(float freq, float duration, float volume) {
    int sampleRate = 44100;
    int n = (int)(sampleRate * duration);
    std::vector<int16_t> samples(n);
    for (int i = 0; i < n; i++) {
        float t = (float)i / sampleRate;
        float env = 1.0f - t / duration;
        samples[i] = (int16_t)(sinf(2.0f * 3.14159f * freq * t) * env * volume * 32767.0f);
    }
    auto wav = makeWav(samples);
    PlaySoundW((LPCWSTR)wav.data(), nullptr, SND_MEMORY | SND_ASYNC);
}

void SoundPlayer::playClick() {
    playTone(1200.0f, 0.05f, 0.2f);
}

void SoundPlayer::playJump() {
    playTone(400.0f, 0.08f, 0.15f);
}

void SoundPlayer::playMoodChange() {
    playTone(600.0f, 0.06f, 0.2f);
    playTone(900.0f, 0.06f, 0.2f);
}
