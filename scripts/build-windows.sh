#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# build-windows.sh  —  Cross-compila SFOGLI-ACCIO per Windows x64
#
# PREREQUISITI su Arch Linux:
#   sudo pacman -S mingw-w64-gcc cmake ninja
#   yay -S mingw-w64-qt6-base mingw-w64-qt6-tools   # da AUR
#
# In alternativa usa MXE (più completo):
#   https://mxe.cc
#
# Esegui dalla root del progetto: bash scripts/build-windows.sh
# ─────────────────────────────────────────────────────────────────────────────
set -e

APPNAME="SfogliAccio"
VERSION="1.0.0"
BUILDDIR="build-windows"
OUTDIR="dist-windows"
TARGET="x86_64-w64-mingw32"

echo "══════════════════════════════════════════════"
echo "  SFOGLI-ACCIO — Cross-compile Windows x64"
echo "══════════════════════════════════════════════"

# ── Rilevamento toolchain ─────────────────────────────────────────────────────
echo ""
echo "▶ Rilevamento toolchain MinGW..."

# Possibili prefissi MinGW su Arch
for prefix in x86_64-w64-mingw32 x86_64-w64-mingw32.static; do
    if command -v "$prefix-gcc" &>/dev/null; then
        TARGET="$prefix"
        echo "  ✓ Trovato: $TARGET-gcc"
        break
    fi
done

if ! command -v "$TARGET-gcc" &>/dev/null; then
    echo "  ✗ MinGW non trovato."
    echo ""
    echo "  Installa con:"
    echo "    sudo pacman -S mingw-w64-gcc"
    echo ""
    echo "  Per Qt6 Windows da AUR:"
    echo "    yay -S mingw-w64-qt6-base mingw-w64-qt6-tools mingw-w64-qt6-translations"
    echo ""
    echo "  ALTERNATIVA: usa MXE (build environment completo):"
    echo "    git clone https://github.com/mxe/mxe.git ~/mxe"
    echo "    cd ~/mxe && make MXE_TARGETS='x86_64-w64-mingw32.static' qt6-qtbase"
    exit 1
fi

# Cerca Qt6 per Windows
QT_WIN_PREFIX=""
for d in \
    "/usr/$TARGET" \
    "/usr/lib/mxe/usr/$TARGET" \
    "$HOME/mxe/usr/$TARGET" \
    "/opt/mxe/usr/$TARGET"; do
    if [ -f "$d/lib/cmake/Qt6/Qt6Config.cmake" ] || \
       [ -f "$d/qt6/lib/cmake/Qt6/Qt6Config.cmake" ]; then
        QT_WIN_PREFIX="$d"
        echo "  ✓ Qt6 Windows trovato in: $QT_WIN_PREFIX"
        break
    fi
done

if [ -z "$QT_WIN_PREFIX" ]; then
    echo ""
    echo "  ✗ Qt6 per Windows non trovato."
    echo ""
    echo "  Opzione 1 — AUR (Arch):"
    echo "    yay -S mingw-w64-qt6-base mingw-w64-qt6-tools"
    echo ""
    echo "  Opzione 2 — MXE (raccomandato, più stabile):"
    echo "    git clone https://github.com/mxe/mxe.git ~/mxe"
    echo "    cd ~/mxe"
    echo "    make MXE_TARGETS='x86_64-w64-mingw32.static' \\"
    echo "         qt6-qtbase qt6-qttools"
    echo ""
    echo "  Opzione 3 — Installa Qt6 direttamente su Windows e compila lì."
    exit 1
fi

# Cerca MuPDF per Windows
MUPDF_WIN=""
for d in \
    "/usr/$TARGET" \
    "$QT_WIN_PREFIX" \
    "/usr/lib/mxe/usr/$TARGET"; do
    if [ -f "$d/lib/libmupdf.a" ] || [ -f "$d/lib/mupdf.lib" ]; then
        MUPDF_WIN="$d"
        echo "  ✓ MuPDF Windows trovato in: $MUPDF_WIN"
        break
    fi
done

if [ -z "$MUPDF_WIN" ]; then
    echo ""
    echo "  ✗ MuPDF per Windows non trovato."
    echo "  Scarica i binari precompilati da: https://mupdf.com/releases/"
    echo "  oppure compila MuPDF con MinGW:"
    echo "    git clone --recursive git://git.ghostscript.com/mupdf.git"
    echo "    cd mupdf"
    echo "    make HAVE_X11=no HAVE_GLFW=no CC=$TARGET-gcc AR=$TARGET-ar \\"
    echo "         prefix=/usr/$TARGET install"
    exit 1
fi

# ── Toolchain file ────────────────────────────────────────────────────────────
echo ""
echo "▶ Scrittura toolchain CMake..."

cat > "$BUILDDIR-toolchain.cmake" << EOF
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   $TARGET-gcc)
set(CMAKE_CXX_COMPILER $TARGET-g++)
set(CMAKE_RC_COMPILER  $TARGET-windres)

set(CMAKE_FIND_ROOT_PATH $QT_WIN_PREFIX $MUPDF_WIN)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(Qt6_DIR $QT_WIN_PREFIX/lib/cmake/Qt6)
EOF

echo "  ✓ Toolchain: $BUILDDIR-toolchain.cmake"

# ── Compila ───────────────────────────────────────────────────────────────────
echo ""
echo "▶ Compilazione..."

cmake -B "$BUILDDIR" \
      -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE="$BUILDDIR-toolchain.cmake" \
      -DCMAKE_INSTALL_PREFIX="$OUTDIR"

cmake --build "$BUILDDIR" --parallel

# ── Raccolta DLL ──────────────────────────────────────────────────────────────
echo ""
echo "▶ Raccolta DLL Qt e runtime..."

mkdir -p "$OUTDIR"
cmake --install "$BUILDDIR"

# Copia DLL Qt necessarie
QT_BIN="$QT_WIN_PREFIX/bin"
for dll in \
    Qt6Core Qt6Gui Qt6Widgets Qt6PrintSupport Qt6Concurrent \
    libgcc_s_seh-1 libstdc++-6 libwinpthread-1; do
    src=$(find "$QT_WIN_PREFIX" /usr/$TARGET -name "${dll}.dll" 2>/dev/null | head -1)
    if [ -n "$src" ]; then
        cp "$src" "$OUTDIR/bin/" 2>/dev/null && echo "  ✓ $dll.dll" || true
    fi
done

# Plugin Qt per Windows
for plugin_dir in platforms styles imageformats; do
    src_dir=$(find "$QT_WIN_PREFIX" -type d -name "$plugin_dir" 2>/dev/null | head -1)
    if [ -n "$src_dir" ]; then
        mkdir -p "$OUTDIR/bin/$plugin_dir"
        cp "$src_dir"/*.dll "$OUTDIR/bin/$plugin_dir/" 2>/dev/null || true
    fi
done

# ── Crea installer NSIS ───────────────────────────────────────────────────────
echo ""
echo "▶ Creazione installer NSIS..."

if ! command -v makensis &>/dev/null; then
    echo "  ⚠ NSIS non trovato. Installa con: sudo pacman -S nsis"
    echo "  Salto la creazione dell'installer. I file sono in $OUTDIR/"
else
    cat > "$BUILDDIR/installer.nsi" << 'NSIS'
!define APPNAME "SFOGLI-ACCIO"
!define VERSION "1.0.0"
!define PUBLISHER "ACCIO"
!define APPEXE "sfogliaccio.exe"

Name "${APPNAME} ${VERSION}"
OutFile "SfogliAccio-${VERSION}-Setup.exe"
InstallDir "$PROGRAMFILES64\${APPNAME}"
InstallDirRegKey HKCU "Software\${APPNAME}" ""
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!include "MUI2.nsh"
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Italian"

Section "Principale" SecMain
    SetOutPath "$INSTDIR"
    File /r "${OUTDIR}\bin\*.*"

    ; Associazione file .pdf
    WriteRegStr HKCU "Software\Classes\.pdf\OpenWithProgids" "SfogliAccio.PDF" ""
    WriteRegStr HKCU "Software\Classes\SfogliAccio.PDF" "" "Documento PDF"
    WriteRegStr HKCU "Software\Classes\SfogliAccio.PDF\shell\open\command" "" '"$INSTDIR\${APPEXE}" "%1"'

    ; Shortcut desktop
    CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\${APPEXE}"

    ; Menu Start
    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\${APPEXE}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Disinstalla.lnk" "$INSTDIR\Uninstall.exe"

    ; Uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
        "DisplayName" "${APPNAME}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
        "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
        "DisplayVersion" "${VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
        "Publisher" "${PUBLISHER}"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\*.*"
    RMDir /r "$INSTDIR"
    Delete "$DESKTOP\${APPNAME}.lnk"
    RMDir /r "$SMPROGRAMS\${APPNAME}"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
    DeleteRegKey HKCU "Software\Classes\SfogliAccio.PDF"
SectionEnd
NSIS

    # Sostituisci placeholder nel NSI
    sed -i "s|\${OUTDIR}|$(pwd)/$OUTDIR|g" "$BUILDDIR/installer.nsi"

    makensis "$BUILDDIR/installer.nsi"

    if [ -f "SfogliAccio-${VERSION}-Setup.exe" ]; then
        echo ""
        echo "══════════════════════════════════════════════"
        echo "  ✅ Installer creato: SfogliAccio-${VERSION}-Setup.exe"
        echo "  Dimensione: $(du -sh SfogliAccio-${VERSION}-Setup.exe | cut -f1)"
        echo "══════════════════════════════════════════════"
    fi
fi

echo ""
echo "══════════════════════════════════════════════"
echo "  ✅ Build Windows completata!"
echo "  File in: $OUTDIR/"
echo "══════════════════════════════════════════════"
