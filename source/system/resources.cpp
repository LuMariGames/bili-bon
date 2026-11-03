#include "resources.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

namespace {
bool isDirectory(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}
void collectSfx(const std::string& folder, std::vector<std::string>& files) {
    DIR* dir = opendir(folder.c_str());
    if (!dir) return;
    while (dirent* ent = readdir(dir)) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        std::string path = folder + "/" + ent->d_name;
        if (isDirectory(path)) collectSfx(path, files);
        else {
            const auto n = path.size();
            if (n >= 4 && path.substr(n - 4) == ".ogg") files.push_back(path);
        }
    }
    closedir(dir);
}
}
bool Resources::load() {
    titleSheet = C2D_SpriteSheetLoad("sdmc:/bili-bon/title.t3x");
    if (titleSheet) {
        titleImage = C2D_SpriteSheetGetImage(titleSheet, 0);
        hasTitleSprite = titleImage.tex != nullptr;
    }
    std::vector<std::string> sfxFiles;
    collectSfx("sdmc:/bili-bon/sfx", sfxFiles);
    for (const auto& path : sfxFiles) {
        OggPcm clip;
        if (decodeOgg(path, clip)) soundEffects.push_back(std::move(clip));
    }
    return true; // Missing optional assets do not prevent the title screen from opening.
}
void Resources::unload() {
    soundEffects.clear();
    if (titleSheet) {
        C2D_SpriteSheetFree(titleSheet);
        titleSheet = nullptr;
    }
    hasTitleSprite = false;
}
