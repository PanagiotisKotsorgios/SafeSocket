#!/usr/bin/env bash
# gui/scripts/fetch_imgui.sh
# Downloads the required Dear ImGui source files into gui/third_party/imgui/
# Run once after cloning the repository.
#
# Requirements: git or curl

set -e

IMGUI_TAG="v1.90.5"
IMGUI_REPO="https://github.com/ocornut/imgui.git"
DEST="$(cd "$(dirname "$0")/.." && pwd)/third_party/imgui"

echo "[fetch_imgui] Target directory: $DEST"

if [ -f "$DEST/imgui.h" ]; then
    echo "[fetch_imgui] imgui.h already present — skipping download."
    echo "[fetch_imgui] To re-fetch, delete $DEST and run this script again."
    exit 0
fi

mkdir -p "$DEST/backends"

# Preferred method: sparse clone (fast, bandwidth-efficient)
if command -v git &>/dev/null; then
    echo "[fetch_imgui] Cloning $IMGUI_REPO at tag $IMGUI_TAG ..."
    TMP=$(mktemp -d)
    git clone --depth 1 --branch "$IMGUI_TAG" "$IMGUI_REPO" "$TMP/imgui"

    FILES=(
        imgui.h imgui.cpp imgui_internal.h imconfig.h
        imstb_rectpack.h imstb_textedit.h imstb_truetype.h
        imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp
    )
    BACKENDS=(
        backends/imgui_impl_sdl2.h  backends/imgui_impl_sdl2.cpp
        backends/imgui_impl_opengl3.h backends/imgui_impl_opengl3.cpp
        backends/imgui_impl_opengl3_loader.h
    )

    for f in "${FILES[@]}"; do
        cp "$TMP/imgui/$f" "$DEST/$f"
    done
    for f in "${BACKENDS[@]}"; do
        cp "$TMP/imgui/$f" "$DEST/$f"
    done

    rm -rf "$TMP"
    echo "[fetch_imgui] Done. Files written to $DEST"

elif command -v curl &>/dev/null; then
    # Fallback: download individual files via GitHub raw
    BASE="https://raw.githubusercontent.com/ocornut/imgui/$IMGUI_TAG"
    echo "[fetch_imgui] git not found; using curl to download individual files ..."

    for f in \
        imgui.h imgui.cpp imgui_internal.h imconfig.h \
        imstb_rectpack.h imstb_textedit.h imstb_truetype.h \
        imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp
    do
        echo "  $f"
        curl -sSfL "$BASE/$f" -o "$DEST/$f"
    done

    for f in \
        imgui_impl_sdl2.h imgui_impl_sdl2.cpp \
        imgui_impl_opengl3.h imgui_impl_opengl3.cpp \
        imgui_impl_opengl3_loader.h
    do
        echo "  backends/$f"
        curl -sSfL "$BASE/backends/$f" -o "$DEST/backends/$f"
    done

    echo "[fetch_imgui] Done."
else
    echo "[fetch_imgui] ERROR: neither git nor curl found."
    echo "Please install git or curl, or manually copy the Dear ImGui files."
    echo "Required files are listed in gui/third_party/imgui/FILES.txt"
    exit 1
fi
