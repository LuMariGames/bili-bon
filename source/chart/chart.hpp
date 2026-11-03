#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Note {
    int32_t scrollSpeed; // Usually BPM.
    float timing;         // Seconds from the start of the song.
};
static_assert(sizeof(Note) == 8, "A chart note must be exactly 8 bytes.");

struct Chart {
    std::vector<Note> notes;
};

bool fileExists(const std::string& path);
bool saveChart(const std::string& path, const Chart& chart);
bool loadChart(const std::string& path, Chart& chart);
