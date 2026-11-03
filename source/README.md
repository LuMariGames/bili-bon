# bili-bon source foundation (Nintendo 3DS)

This folder contains the first implementation pass. Build files are intentionally not included or changed.

## Expected SD card assets

- `sdmc:/bili-bon/title.t3x`: Citro2D sprite sheet; image index 0 should contain the combined title logo and character.
- `sdmc:/bili-bon/*.ogg` and nested folders: selectable music.
- `sdmc:/bili-bon/sfx/*.ogg`: OGG sound effects loaded by the resource loader; the `sfx` folder is excluded from the music list.
- Selecting a song creates `fumen.bin` beside that song, unless that file already exists.

## Dependencies

The project build must link the devkitPro 3DS libraries for libctru, citro2d/citro3d, libogg and Tremor (vorbisidec). Header path used for Tremor is `<tremor/ivorbisfile.h>`.

## Important implementation notes

- `fumen.bin` is a raw array of 8-byte records: signed 32-bit scroll speed followed by a 32-bit float timestamp in seconds. Its record count is inferred from file size; there is no header.
- The first chart generator is a placeholder 120 BPM grid, not waveform-based beat detection. The `generatePlaceholderChart` interface is isolated so a later aubio or FFT/onset analyzer can replace it.
- OGG decoding currently decodes a selected track fully into RAM before chart generation. This is suitable for getting the first foundation in place but should be changed to streaming/analysis windows before very long tracks are supported.
- The title screen remains black if `title.t3x` is missing; the optional sprite does not block startup.
- AudioPlayer exposes one-shot playback on NDSP channels. The loaded SFX clips are decoded and retained, but this initial scene scaffold does not yet bind them to UI events.
- The render loop uses `C3D_FrameEnd(0)` to finish a frame, then waits for VBlank. It is not used as a standalone sleep call.
