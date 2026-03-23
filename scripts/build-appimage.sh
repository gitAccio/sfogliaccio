#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# build-appimage.sh  —  Crea un AppImage di SFOGLI-ACCIO per Linux x86_64
# Esegui dalla root del progetto: bash scripts/build-appimage.sh
# ─────────────────────────────────────────────────────────────────────────────
set -e

APPNAME="SfogliAccio"
VERSION="1.0.0"
ARCH="x86_64"
BUILDDIR="build-appimage"
APPDIR="$BUILDDIR/$APPNAME.AppDir"

echo "══════════════════════════════════════════════"
echo "  SFOGLI-ACCIO — Build AppImage v$VERSION"
echo "══════════════════════════════════════════════"

# ── 1. Dipendenze ─────────────────────────────────────────────────────────────
echo ""
echo "▶ Controllo dipendenze..."

check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo "  ✗ $1 non trovato — installa con: $2"
        exit 1
    else
        echo "  ✓ $1"
    fi
}

check_dep cmake    "sudo pacman -S cmake"
check_dep ninja    "sudo pacman -S ninja"
check_dep patchelf "sudo pacman -S patchelf"

# Scarica linuxdeploy e plugin Qt se non presenti
mkdir -p "$BUILDDIR"

if [ ! -f "$BUILDDIR/linuxdeploy-x86_64.AppImage" ]; then
    echo ""
    echo "▶ Download linuxdeploy..."
    curl -L -o "$BUILDDIR/linuxdeploy-x86_64.AppImage" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$BUILDDIR/linuxdeploy-x86_64.AppImage"
fi

if [ ! -f "$BUILDDIR/linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    echo "▶ Download linuxdeploy-plugin-qt..."
    curl -L -o "$BUILDDIR/linuxdeploy-plugin-qt-x86_64.AppImage" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$BUILDDIR/linuxdeploy-plugin-qt-x86_64.AppImage"
fi

# ── 2. Compila Release ────────────────────────────────────────────────────────
echo ""
echo "▶ Compilazione Release..."

cmake -B "$BUILDDIR/cmake-build" \
      -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$BUILDDIR/cmake-build" --parallel

# ── 3. Install in AppDir ──────────────────────────────────────────────────────
echo ""
echo "▶ Installazione in AppDir..."
rm -rf "$APPDIR"
DESTDIR="$(pwd)/$APPDIR" cmake --install "$BUILDDIR/cmake-build"

# ── 4. Icona ──────────────────────────────────────────────────────────────────
echo ""
echo "▶ Icona..."
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

if [ -f "resources/icon.png" ]; then
    cp resources/icon.png "$APPDIR/usr/share/icons/hicolor/256x256/apps/sfogliaccio.png"
    echo "  ✓ Icona copiata da resources/icon.png"
else
    echo "  ⚠ Nessuna icona trovata, uso placeholder"
    python3 -c "
import struct, zlib
def chunk(t,d):
    c=struct.pack('>I',len(d))+t+d
    return c+struct.pack('>I',zlib.crc32(c[4:])&0xffffffff)
ihdr=struct.pack('>IIBBBBB',256,256,8,2,0,0,0)
raw=b''.join(b'\x00'+bytes([226,245,66]*256) for _ in range(256))
idat=zlib.compress(raw)
data=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',ihdr)+chunk(b'IDAT',idat)+chunk(b'IEND',b'')
open('$APPDIR/usr/share/icons/hicolor/256x256/apps/sfogliaccio.png','wb').write(data)
"
fi

# Desktop file e symlink
mkdir -p "$APPDIR/usr/share/applications"
cp resources/sfogliaccio.desktop "$APPDIR/usr/share/applications/"
ln -sf usr/share/applications/sfogliaccio.desktop "$APPDIR/sfogliaccio.desktop" 2>/dev/null || true
ln -sf usr/share/icons/hicolor/256x256/apps/sfogliaccio.png "$APPDIR/sfogliaccio.png" 2>/dev/null || true

# ── 5. Crea AppImage ──────────────────────────────────────────────────────────
echo ""
echo "▶ Creazione AppImage..."

export QMAKE=$(which qmake6 2>/dev/null || which qmake 2>/dev/null)
export OUTPUT="${APPNAME}-${VERSION}-${ARCH}.AppImage"
export NO_STRIP=1
export LINUXDEPLOY_PLUGIN_QT_NO_STRIP=1

# Il plugin Qt deve essere nella stessa cartella di linuxdeploy
cd "$BUILDDIR"
./linuxdeploy-x86_64.AppImage \
    --appdir "$APPNAME.AppDir" \
    --plugin qt \
    --output appimage
cd ..

# Trova l'AppImage generata
FOUND=$(find "$BUILDDIR" . -maxdepth 2 -name "*.AppImage" \
    ! -name "linuxdeploy*" | head -1)

if [ -n "$FOUND" ]; then
    DEST="${APPNAME}-${VERSION}-${ARCH}.AppImage"
    [ "$FOUND" != "./$DEST" ] && mv "$FOUND" "$DEST" 2>/dev/null || true
    echo ""
    echo "══════════════════════════════════════════════"
    echo "  ✅ AppImage creata: $DEST"
    echo "  Dimensione: $(du -sh $DEST | cut -f1)"
    echo ""
    echo "  Esegui con:"
    echo "    chmod +x $DEST && ./$DEST"
    echo "══════════════════════════════════════════════"
else
    echo "  ✗ AppImage non trovata. Controlla l'output sopra."
    exit 1
fi