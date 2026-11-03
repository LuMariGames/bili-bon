#include "ogg_audio.hpp"
#include <tremor/ivorbisfile.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

ndspWaveBuf AudioPlayer::waveBuffers[AudioPlayer::ChannelCount]{};
int16_t* AudioPlayer::linearBuffers[AudioPlayer::ChannelCount]{};

bool decodeOgg(const std::string& path, OggPcm& out) {
    OggVorbis_File vf;
    std::memset(&vf, 0, sizeof(vf));
    if (ov_fopen(path.c_str(), &vf) != 0) return false;
    vorbis_info* info = ov_info(&vf, -1);
    if (!info || info->rate <= 0 || (info->channels != 1 && info->channels != 2)) {
        ov_clear(&vf);
        return false;
    }

    OggPcm decoded;
    decoded.sampleRate = static_cast<unsigned int>(info->rate);
    decoded.channels = info->channels;
    decoded.durationSeconds = ov_time_total(&vf, -1);
    char buffer[8192];
    int bitstream = 0;
    long bytesRead = 0;
    while ((bytesRead = ov_read(&vf, buffer, sizeof(buffer), &bitstream)) > 0) {
        if (bytesRead % static_cast<long>(sizeof(int16_t)) != 0) {
            ov_clear(&vf);
            return false;
        }
        const int16_t* samples = reinterpret_cast<const int16_t*>(buffer);
        decoded.samples.insert(decoded.samples.end(), samples,
                               samples + bytesRead / sizeof(int16_t));
    }
    ov_clear(&vf);
    if (bytesRead < 0 || decoded.samples.empty()) return false;
    out = std::move(decoded);
    return true;
}

bool AudioPlayer::play(const OggPcm& clip, int channel) {
    if (channel < 0 || channel >= ChannelCount || clip.samples.empty() ||
        (clip.channels != 1 && clip.channels != 2)) return false;
    update();
    if (linearBuffers[channel]) return false; // Do not replace a buffer still in use.
    const size_t bytes = clip.samples.size() * sizeof(int16_t);
    int16_t* pcm = static_cast<int16_t*>(linearAlloc(bytes));
    if (!pcm) return false;
    std::memcpy(pcm, clip.samples.data(), bytes);

    ndspWaveBuf& wave = waveBuffers[channel];
    std::memset(&wave, 0, sizeof(wave));
    wave.data_vaddr = pcm;
    wave.nsamples = static_cast<u32>(clip.samples.size() / clip.channels);
    wave.looping = false;
    DSP_FlushDataCache(pcm, bytes);

    ndspChnReset(channel);
    ndspChnSetInterp(channel, NDSP_INTERP_LINEAR);
    ndspChnSetRate(channel, static_cast<float>(clip.sampleRate));
    ndspChnSetFormat(channel, clip.channels == 1 ? NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_STEREO_PCM16);
    ndspChnWaveBufAdd(channel, &wave);
    ndspChnSetPaused(channel, false);
    linearBuffers[channel] = pcm;
    return true;
}
void AudioPlayer::stop(int channel) {
    if (channel < 0 || channel >= ChannelCount) return;
    ndspChnReset(channel);
    if (linearBuffers[channel]) linearFree(linearBuffers[channel]);
    linearBuffers[channel] = nullptr;
    std::memset(&waveBuffers[channel], 0, sizeof(waveBuffers[channel]));
}
void AudioPlayer::update() {
    for (int i = 0; i < ChannelCount; ++i) {
        if (linearBuffers[i] && waveBuffers[i].status == NDSP_WBUF_DONE) {
            linearFree(linearBuffers[i]);
            linearBuffers[i] = nullptr;
            std::memset(&waveBuffers[i], 0, sizeof(waveBuffers[i]));
        }
    }
}
void AudioPlayer::shutdown() {
    for (int i = 0; i < ChannelCount; ++i) stop(i);
}
