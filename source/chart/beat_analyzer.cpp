#include "beat_analyzer.hpp"
#include <cmath>
#include <limits>

bool generatePlaceholderChart(double durationSeconds, Chart& out) {
    if (!(durationSeconds > 0.0) || !std::isfinite(durationSeconds)) return false;
    constexpr int32_t bpm = 120;
    constexpr double beatSeconds = 60.0 / bpm;
    const double countD = std::ceil(durationSeconds / beatSeconds);
    if (countD < 1.0 || countD > 100000.0) return false;
    Chart chart;
    chart.notes.reserve(static_cast<size_t>(countD));
    for (size_t i = 0; i < static_cast<size_t>(countD); ++i) {
        const double time = i * beatSeconds;
        if (time >= durationSeconds) break;
        chart.notes.push_back(Note{bpm, static_cast<float>(time)});
    }
    out = std::move(chart);
    return !out.notes.empty();
}
