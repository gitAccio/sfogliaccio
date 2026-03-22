# SFOGLI-ACCIO — Guida alla distribuzione

## Linux — AppImage

Un singolo file che gira su qualsiasi distro Linux (Ubuntu, Fedora, Arch, ecc.)
senza installare niente.

### Prerequisiti

```bash
# Arch Linux
sudo pacman -S cmake ninja patchelf

# Ubuntu/Debian
sudo apt install cmake ninja-build patchelf curl
```

### Build

```bash
cd vista-cpp
bash scripts/build-appimage.sh
```

Genera: `SfogliAccio-1.0.0-x86_64.AppImage`

### Distribuzione

```bash
chmod +x SfogliAccio-1.0.0-x86_64.AppImage
./SfogliAccio-1.0.0-x86_64.AppImage
```

### Installazione di sistema (opzionale)

```bash
# Copia il binario
sudo cp SfogliAccio-1.0.0-x86_64.AppImage /usr/local/bin/sfogliaccio
sudo chmod +x /usr/local/bin/sfogliaccio

# Desktop integration
cp resources/sfogliaccio.desktop ~/.local/share/applications/
# Aggiorna database
update-desktop-database ~/.local/share/applications/
```

---

## Linux — Pacchetto Arch (PKGBUILD)

Per distribuire su Arch Linux / AUR:

```bash
# Crea directory pacchetto
mkdir sfogliaccio-pkg && cd sfogliaccio-pkg

cat > PKGBUILD << 'EOF'
pkgname=sfogliaccio
pkgver=1.0.0
pkgrel=1
pkgdesc="Lettore PDF veloce, moderno e gratuito"
arch=('x86_64')
license=('MIT')
depends=('qt6-base' 'mupdf' 'libmupdf')
makedepends=('cmake' 'ninja')
source=("$pkgname-$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$pkgname-$pkgver"
    cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
}

package() {
    cd "$pkgname-$pkgver"
    DESTDIR="$pkgdir" cmake --install build
}
EOF

makepkg -si
```

---

## Windows — Cross-compile da Linux

### Prerequisiti

```bash
# Compilatore MinGW
sudo pacman -S mingw-w64-gcc cmake ninja nsis

# Qt6 per Windows (da AUR)
yay -S mingw-w64-qt6-base mingw-w64-qt6-tools

# MuPDF per Windows — compila da sorgente:
git clone --recursive git://git.ghostscript.com/mupdf.git
cd mupdf
make HAVE_X11=no HAVE_GLFW=no HAVE_CURL=no \
     CC=x86_64-w64-mingw32-gcc \
     AR=x86_64-w64-mingw32-ar \
     WINDRES=x86_64-w64-mingw32-windres \
     prefix=/usr/x86_64-w64-mingw32 \
     install
```

### Build + Installer

```bash
cd vista-cpp
bash scripts/build-windows.sh
```

Genera: `SfogliAccio-1.0.0-Setup.exe`

---

## Windows — Compila direttamente su Windows

Se hai una macchina Windows disponibile è più semplice:

### Prerequisiti Windows

1. **Qt6** — https://www.qt.io/download-qt-installer
   - Seleziona: Qt 6.x → MinGW 64-bit
   
2. **MuPDF** — https://mupdf.com/releases/
   - Scarica i binari Windows o compila con MSVC

3. **CMake** — https://cmake.org/download/

4. **Ninja** — https://ninja-build.org/

5. **NSIS** (per installer) — https://nsis.sourceforge.io/

### Build su Windows

```batch
mkdir build-win && cd build-win
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release ^
         -DCMAKE_PREFIX_PATH="C:\Qt\6.x\mingw_64"
ninja
```

### Raccolta DLL con windeployqt

```batch
cd build-win
windeployqt --release sfogliaccio.exe
```

Poi crea l'installer con NSIS usando lo script `scripts/installer.nsi`.

---

## Struttura output finale

```
dist/
├── SfogliAccio-1.0.0-x86_64.AppImage   ← Linux (tutto in uno)
└── SfogliAccio-1.0.0-Setup.exe         ← Windows installer
```

L'installer Windows:
- Installa in `C:\Program Files\SFOGLI-ACCIO\`
- Crea shortcut sul desktop e nel menu Start
- Associa i file `.pdf` all'app
- Include disinstallatore
