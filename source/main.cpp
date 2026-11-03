#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

#include "system/resources.hpp"
#include "audio/ogg_audio.hpp"
#include "chart/chart.hpp"
#include "chart/beat_analyzer.hpp"

namespace {
enum class Scene { Loading, Title, MusicSelect };
Scene scene = Scene::Loading;
Resources resources;
std::vector<std::string> musicFiles;
int selectedMusic = 0;
int listOffset = 0;
int loadFrames = 0;
std::string message;
C2D_TextBuf textBuffer;
C2D_Text textLine[10];

bool isDirectory(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}
bool hasOggExtension(const std::string& path) {
    if (path.size() < 4) return false;
    std::string ext = path.substr(path.size() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return ext == ".ogg";
}
void scanMusic(const std::string& directory, std::vector<std::string>& out) {
    DIR* dir = opendir(directory.c_str());
    if (!dir) return;
    while (dirent* ent = readdir(dir)) {
        const char* name = ent->d_name;
        if (!strcmp(name, ".") || !strcmp(name, "..")) continue;
        const std::string path = directory + "/" + name;
        if (isDirectory(path)) {
            // Sound effects live here and must not appear as selectable songs.
            if (!strcmp(name, "sfx")) continue;
            scanMusic(path, out);
        } else if (hasOggExtension(path)) {
            out.push_back(path);
        }
    }
    closedir(dir);
}
std::string fileName(const std::string& path) {
    const std::string::size_type pos = path.find_last_of('/');
    return pos == std::string::npos ? path : path.substr(pos + 1);
}
std::string parentDirectory(const std::string& path) {
    const std::string::size_type pos = path.find_last_of('/');
    return pos == std::string::npos ? "." : path.substr(0, pos);
}
void drawText(GFX_BOTTOM orTop, const char* text, float x, float y, float scale = 0.55f) {
    C2D_Text parsed;
    C2D_TextParse(&parsed, textBuffer, text);
    C2D_TextOptimize(&parsed);
    C2D_DrawText(&parsed, C2D_AtBaseline, x, y, 0.5f, scale, scale);
}
void drawTitleTop() {
    C2D_TargetClear(resources.topTarget, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(resources.topTarget);
    if (resources.hasTitleSprite) {
        const float w = resources.titleImage.subtex->width;
        const float h = resources.titleImage.subtex->height;
        C2D_DrawImageAt(resources.titleImage, (400.0f - w) * 0.5f,
                        (240.0f - h) * 0.5f, 0.5f, nullptr, 1.0f, 1.0f);
    }
}
void drawBottomPrompt(const char* prompt) {
    C2D_TargetClear(resources.bottomTarget, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(resources.bottomTarget);
    drawText(GFX_BOTTOM, prompt, 35.0f, 125.0f);
}
void render() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    textBuffer = C2D_TextBufNew(4096);
    switch (scene) {
    case Scene::Loading:
        C2D_TargetClear(resources.topTarget, C2D_Color32(0, 0, 0, 255));
        C2D_TargetClear(resources.bottomTarget, C2D_Color32(0, 0, 0, 255));
        break;
    case Scene::Title:
        drawTitleTop();
        drawBottomPrompt("STARTボタンで開始");
        break;
    case Scene::MusicSelect: {
        drawTitleTop();
        C2D_TargetClear(resources.bottomTarget, C2D_Color32(0, 0, 0, 255));
        C2D_SceneBegin(resources.bottomTarget);
        if (musicFiles.empty()) {
            drawText(GFX_BOTTOM, "音楽ファイルを入れて下さい", 12.0f, 120.0f, 0.5f);
        } else {
            const int visible = 8;
            for (int i = 0; i < visible && listOffset + i < (int)musicFiles.size(); ++i) {
                const int index = listOffset + i;
                std::string label = (index == selectedMusic ? "> " : "  ") + fileName(musicFiles[index]);
                if (label.size() > 42) label = label.substr(0, 39) + "...";
                drawText(GFX_BOTTOM, label.c_str(), 8.0f, 28.0f + i * 25.0f, 0.45f);
            }
            if (!message.empty())
                drawText(GFX_BOTTOM, message.c_str(), 8.0f, 225.0f, 0.38f);
        }
        break;
    }
    }
    C2D_TextBufDelete(textBuffer);
    textBuffer = nullptr;
    C3D_FrameEnd(0);
}
bool selectSong(const std::string& path) {
    const std::string folder = parentDirectory(path);
    const std::string chartPath = folder + "/fumen.bin";
    Chart chart;
    if (fileExists(chartPath)) {
        if (!loadChart(chartPath, chart)) {
            message = "fumen.binを読み込めません";
            return false;
        }
        message = "既存のfumen.binを読み込みました";
        return true;
    }
    OggPcm pcm;
    if (!decodeOgg(path, pcm)) {
        message = "OGGを読み込めません";
        return false;
    }
    // First implementation uses a regular 120 BPM grid. Audio-onset analysis
    // is deliberately isolated behind this interface for later replacement.
    if (!generatePlaceholderChart(pcm.durationSeconds, chart)) {
        message = "譜面を生成できません";
        return false;
    }
    if (!saveChart(chartPath, chart)) {
        message = "fumen.binを保存できません";
        return false;
    }
    message = "譜面を生成し、fumen.binに保存しました";
    return true;
}
}

int main(int, char**) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, nullptr);
    ndspInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    resources.topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    resources.bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // The loading screen intentionally has no text or graphics.
    render();
    resources.load();
    scanMusic("sdmc:/bili-bon", musicFiles);
    std::sort(musicFiles.begin(), musicFiles.end());
    scene = Scene::Title;

    while (aptMainLoop()) {
        hidScanInput();
        const u32 down = hidKeysDown();
        AudioPlayer::update();

        if (scene == Scene::Title && (down & KEY_START)) {
            scene = Scene::MusicSelect;
            musicFiles.clear();
            scanMusic("sdmc:/bili-bon", musicFiles);
            std::sort(musicFiles.begin(), musicFiles.end());
            selectedMusic = 0;
            listOffset = 0;
            message.clear();
        } else if (scene == Scene::MusicSelect && !musicFiles.empty()) {
            if (down & KEY_UP) selectedMusic = std::max(0, selectedMusic - 1);
            if (down & KEY_DOWN) selectedMusic = std::min((int)musicFiles.size() - 1, selectedMusic + 1);
            if (selectedMusic < listOffset) listOffset = selectedMusic;
            if (selectedMusic >= listOffset + 8) listOffset = selectedMusic - 7;
            if (down & KEY_B) { scene = Scene::Title; message.clear(); }
            if (down & KEY_A) selectSong(musicFiles[selectedMusic]);
        } else if (scene == Scene::MusicSelect && (down & KEY_B)) {
            scene = Scene::Title;
        }

        render();
        gspWaitForVBlank();
    }

    resources.unload();
    AudioPlayer::shutdown();
    ndspExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
