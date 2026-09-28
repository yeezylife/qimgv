# AGENTS.md — qimgv

Qt6 + C++23 image viewer. Single CMake project, no tests/linter/formatter config in repo.

## Build (canonical: MSYS2 CLANG64 on Windows)

Deps (clang64): `qt6-base qt6-svg qt6-imageformats qt6-tools kimageformats libjxl libraw opencv lcms2 libavif libheif` + (`mpv kwindowsystem` for video/strict builds).

```bash
# Release (matches qimgv-qt6-build.yml; VIDEO off in release, x86-64-v3 only: Haswell+)
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCMAKE_CXX_EXTENSIONS=OFF \
  -DVIDEO_SUPPORT=OFF -DOPENCV_SUPPORT=ON -DCMAKE_PREFIX_PATH=/clang64 \
  -DCMAKE_CXX_FLAGS="-DNDEBUG -DQT_NO_DEBUG_OUTPUT -O3 -march=x86-64-v3 -flto=thin -fstrict-aliasing -fvisibility=hidden -fvisibility-inlines-hidden -ffunction-sections -fdata-sections"
cmake --build build --config Release

# Debug strict check (matches qimgv-CI-Strict-Check.yml / debug.yml: -Werror + ASan/UBSan)
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCMAKE_CXX_EXTENSIONS=OFF \
  -DVIDEO_SUPPORT=ON -DOPENCV_SUPPORT=ON -DCMAKE_PREFIX_PATH=/clang64 \
  -DCMAKE_CXX_FLAGS="-O0 -g3 -fno-omit-frame-pointer -march=x86-64-v3 -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wold-style-cast -Wcast-qual -Wcast-align -Wzero-as-null-pointer-constant -Wnon-virtual-dtor -Woverloaded-virtual -Wsuggest-override -Wshadow -Wundef -Wdouble-promotion -Wformat=2 -fsanitize=address,undefined"
cmake --build build --config Debug
```

Options: `-DVIDEO_SUPPORT` (defines `USE_MPV`), `-DOPENCV_SUPPORT` (defines `USE_OPENCV`, adds `qimgv/3rdparty/QtOpenCV`), `-DKDE_SUPPORT=OFF` (defines `USE_KDE_BLUR`). No `ctest`, no test suite — verify by compiling.

## Architecture

- Entry: `qimgv/main.cpp` → `Core` (`qimgv/core.cpp|h`) orchestrates everything → `MW` main window (`qimgv/gui/mainwindow.h`).
- `qimgv/components/`: `directorymodel` + `directorypresenter`, `loader/`, `scaler/`, `cache/`, `directorymanager/` (+ `watchers/windows/` only on WIN32), `actionmanager/`, `scriptmanager/`.
- `qimgv/sourcecontainers/`: `Image` / `ImageStatic` / `ImageAnimated` / `Video` / `DocumentInfo`.
- `qimgv/gui/`: `centralwidget`, `viewers/`, `panels/`, `overlays/`, `dialogs/`, `customwidgets/`.
- `qimgv/utils/`: cross-cutting helpers (`settings` adjacent at `qimgv/settings.cpp`, `imagefactory`, `fileoperations`, `inputmap`/`actions`/`shortcutbuilder`, `win11window`).
- `plugins/player_mpv/` is a separate `MODULE` lib (`player_mpv.dll/.so`), not linked into `qimgv`; runtime-loaded from `plugins/` dir. Windows build needs `-DMPV_DIR=...` (include + `lib/x86_64`).
- All module sources are glued via `target_sources(qimgv PRIVATE …)` in each subdir `CMakeLists.txt`; `CMAKE_AUTOMOC/AUTORCC/AUTOUIC` are ON.

## Gotchas

- Windows is portable-layout: `Settings` (`qimgv/settings.cpp`) writes `conf/*.ini` and `cache/` next to the exe; Linux uses `QSettings::NativeFormat`/XDG instead. Don't hardcode one layout.
- `cache/supported_formats.cache` is keyed by `qVersion()`; CI pre-warms it at package time and deletes `conf/` before zipping. Don't "fix" the missing cache by removing the version check.
- Package trimming is intentional: keep only `qjpeg qwebp qgif qico qsvg qtiff kimg_avif kimg_heif kimg_jxl` in `imageformats/`, delete `tls networkinformation generic iconengines` + `Qt6Network.dll`, ship `qt.conf` with `Prefix=. / Plugins=.`. Don't re-add removed plugins.
- `main.cpp` clears `QT_PLUGIN_PATH` on Windows (issue #410), forces `QLocale::c()` (mpv float parsing), and uses `RoundPreferFloor` HiDPI policy. Preserve these.
- Windows linking: `dwmapi`, plus `WIN32_EXECUTABLE + /ENTRY:mainCRTStartup` (MSVC) or `-mwindows` (GCC/Clang); Clang needs `-Wno-shift-negative-value` and `-fuse-ld=lld` (see `qimgv/CMakeLists.txt`).
- Translations: `qt_add_lupdate/lrelease` on `qimgv/res/translations/zh_CN.ts` → `build/qimgv/translations/`; `windeployqt --no-translations` so CI copies them manually.
- Thumbnail subsystem was deliberately removed (README breaking change) — do not reintroduce it.
- Style claim is Google C++ / clang-format but no `.clang-format` exists; match surrounding code, keep `cxx_std_23` with `CXX_EXTENSIONS OFF`.
