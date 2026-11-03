#include "chart.hpp"
#include <cstdio>
#include <sys/stat.h>
#include <limits>

bool fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFREG);
}
bool saveChart(const std::string& path, const Chart& chart) {
    if (chart.notes.size() > std::numeric_limits<size_t>::max() / sizeof(Note)) return false;
    FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    const size_t written = chart.notes.empty() ? 0 :
        std::fwrite(chart.notes.data(), sizeof(Note), chart.notes.size(), file);
    const bool ok = written == chart.notes.size() && std::fflush(file) == 0;
    const bool closeOk = std::fclose(file) == 0;
    return ok && closeOk;
}
bool loadChart(const std::string& path, Chart& chart) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    if (std::fseek(file, 0, SEEK_END) != 0) { std::fclose(file); return false; }
    const long size = std::ftell(file);
    if (size < 0 || size % static_cast<long>(sizeof(Note)) != 0 ||
        std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }
    Chart loaded;
    loaded.notes.resize(static_cast<size_t>(size) / sizeof(Note));
    const size_t read = loaded.notes.empty() ? 0 :
        std::fread(loaded.notes.data(), sizeof(Note), loaded.notes.size(), file);
    const bool ok = read == loaded.notes.size() && std::fclose(file) == 0;
    if (!ok) return false;
    chart = std::move(loaded);
    return true;
}
