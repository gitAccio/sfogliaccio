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
        echo "  ✗ $1 non trovato"
        echo "    Installa con: $2"
        exit 1
    else
        echo "  ✓ $1"
    fi
}

check_dep cmake  "sudo pacman -S cmake"
check_dep ninja  "sudo pacman -S ninja"
check_dep patchelf "sudo pacman -S patchelf"

# Scarica linuxdeploy se non presente
if [ ! -f "$BUILDDIR/linuxdeploy" ]; then
    echo ""
    echo "▶ Download linuxdeploy..."
    mkdir -p "$BUILDDIR"
    curl -L -o "$BUILDDIR/linuxdeploy" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$BUILDDIR/linuxdeploy"

    # Scarica plugin Qt
    curl -L -o "$BUILDDIR/linuxdeploy-plugin-qt" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$BUILDDIR/linuxdeploy-plugin-qt"
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

# Crea icona SVG se non esiste PNG
if [ -f "assets/icon.png" ]; then
    cp assets/icon.png "$APPDIR/usr/share/icons/hicolor/256x256/apps/sfogliaccio.png"
elif [ -f "resources/icon.png" ]; then
    cp resources/icon.png "$APPDIR/usr/share/icons/hicolor/256x256/apps/sfogliaccio.png"
else
    # Genera icona di fallback con ImageMagick se disponibile
    if command -v convert &>/dev/null; then
        convert -size 256x256 xc:"#0d0d0e" \
            -fill "#e2f542" -font DejaVu-Sans-Bold -pointsize 48 \
            -gravity center -annotate 0 "SFOGLI\nACCIO" \
            "$APPDIR/usr/share/icons/hicolor/256x256/apps/sfogliaccio.png"
        echo "  ✓ Icona generata con ImageMagick"
    else
        # Icona minima in PNG (1x1 verde, placeholder)
        echo "  ⚠ Nessuna icona trovata. Aggiungi assets/icon.png (256x256) per un'icona vera."
        # Crea un PNG 256x256 minimo con Python
        python3 -c "
import struct, zlib

def make_png(w, h, color):
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(c[4:]) & 0xffffffff)
    ihdr = struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)
    r, g, b = color
    raw = b''.join(b'\x00' + bytes([r,g,b]*w) for _ in range(h))
    idat = zlib.compress(raw)
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) + chunk(b'IDAT', idat) + chunk(b'IEND', b'')

with open('$APPDIR/usr/share/icons/hicolor/256x256/apps/sfogliaccio.png','wb') as f:
    f.write(make_png(256,256,(226,245,66)))
"
    fi
fi

# Desktop file
mkdir -p "$APPDIR/usr/share/applications"
cp resources/sfogliaccio.desktop "$APPDIR/usr/share/applications/"

# Symlink per linuxdeploy
ln -sf usr/share/applications/sfogliaccio.desktop "$APPDIR/sfogliaccio.desktop" 2>/dev/null || true
ln -sf usr/share/icons/hicolor/256x256/apps/sfogliaccio.png "$APPDIR/sfogliaccio.png" 2>/dev/null || true

# ── 5. Crea AppImage ──────────────────────────────────────────────────────────
echo ""
echo "▶ Creazione AppImage..."

export QMAKE=$(which qmake6 2>/dev/null || which qmake 2>/dev/null)
export OUTPUT="${APPNAME}-${VERSION}-${ARCH}.AppImage"
export NO_STRIP=1
export LINUXDEPLOY_PLUGIN_QT_NO_STRIP=1

cd "$BUILDDIR"
./linuxdeploy \
    --appdir "$APPNAME.AppDir" \
    --plugin qt \
    --output appimage

cd ..

if [ -f "$BUILDDIR/$OUTPUT" ]; then
    mv "$BUILDDIR/$OUTPUT" .
    echo ""
    echo "══════════════════════════════════════════════"
    echo "  ✅ AppImage creata: $OUTPUT"
    echo "  Dimensione: $(du -sh $OUTPUT | cut -f1)"
    echo ""
    echo "  Esegui con:"
    echo "    chmod +x $OUTPUT"
    echo "    ./$OUTPUT"
    echo "══════════════════════════════════════════════"
else
    # linuxdeploy mette l'output nella cwd
    FOUND=$(find . -maxdepth 2 -name "*.AppImage" | head -1)
    if [ -n "$FOUND" ]; then
        echo ""
        echo "  ✅ AppImage creata: $FOUND"
    else
        echo "  ✗ AppImage non trovata. Controlla l'output sopra."
        exit 1
    fi
fi