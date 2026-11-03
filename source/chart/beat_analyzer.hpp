#pragma once
#include "chart.hpp"

// Temporary chart generator: evenly spaced notes on a 120 BPM grid.
// Replace this function with onset/beat analysis later; it intentionally does
// not claim to detect beats from the waveform yet.
bool generatePlaceholderChart(double durationSeconds, Chart& out);
