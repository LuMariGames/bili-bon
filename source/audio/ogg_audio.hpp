#pragma once
#include <3ds.h>
#include <3ds/ndsp/ndsp.h>
#include <cstdint>
#include <string>
#include <vector>

struct OggPcm {
    unsigned int sampleRate = 0;
    int channels = 0;
    double durationSeconds = 0.0;
    std::vector<int16_t> samples; // Interleaved signed 16-bit PCM.
};

bool decodeOgg(const std::string& path, OggPcm& out);

class AudioPlayer {
public:
    static bool play(const OggPcm& clip, int channel);
    static void stop(int channel);
    static void update();
    static void shutdown();
private:
    static constexpr int ChannelCount = 24;
    static ndspWaveBuf waveBuffers[ChannelCount];
    static int16_t* linearBuffers[ChannelCount];
};
