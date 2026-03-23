# SFOGLI-ACCIO — Guida alla distribuzione

---

## 🐧 Linux — AppImage

Il modo più semplice per distribuire su Linux. Un singolo file eseguibile che gira su qualsiasi distro senza installare niente.

### Prerequisiti

```bash
# Arch Linux
sudo pacman -S cmake ninja patchelf

# Ubuntu/Debian
sudo apt install cmake ninja-build patchelf curl
```

### Build

```bash
cd sfogliaccio
bash scripts/build-appimage.sh
```

Lo script scarica automaticamente `linuxdeploy`, compila il progetto in Release e genera:

```
SfogliAccio-1.0.0-x86_64.AppImage
```

### Uso

```bash
chmod +x SfogliAccio-1.0.0-x86_64.AppImage
./SfogliAccio-1.0.0-x86_64.AppImage
```

### Installazione di sistema (opzionale)

```bash
# Installa tramite CMake (modo consigliato)
cd build
sudo cmake --install .
# → binario in /usr/local/bin/sfogliaccio
# → .desktop in /usr/local/share/applications/

# Aggiorna il menu applicazioni
update-desktop-database ~/.local/share/applications/
```

---

## 🐧 Linux — Pacchetto Arch (PKGBUILD)

Per installare tramite pacman con disinstallazione pulita:

```bash
mkdir sfogliaccio-pkg && cd sfogliaccio-pkg

cat > PKGBUILD << 'EOF'
pkgname=sfogliaccio
pkgver=1.0.0
pkgrel=1
pkgdesc="Lettore PDF veloce, moderno e gratuito"
arch=('x86_64')
license=('MIT')
depends=('qt6-base' 'mupdf')
makedepends=('cmake' 'ninja')
source=("$pkgname-$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$pkgname-$pkgver"
    cmake -B build -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
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

## 🪟 Windows — GitHub Actions (raccomandato)

Il metodo più semplice e affidabile. La build avviene su server GitHub gratuitamente.

1. Fai push del codice su GitHub
2. Vai su **Actions → Build SFOGLI-ACCIO Windows → Run workflow**
3. Dopo ~20-30 minuti scarica `SfogliAccio-Setup.exe` dalla sezione **Artifacts**

Il workflow (`build.yml`) gestisce automaticamente:
- Installazione Qt6 via `jurplel/install-qt-action`
- Compilazione MuPDF da sorgente con MSBuild
- Raccolta DLL con `windeployqt`
- Creazione installer NSIS con shortcut, associazione `.pdf` e disinstallatore

---

## 🪟 Windows — Build locale

Se vuoi compilare direttamente sul tuo PC Windows.

### Prerequisiti

| Tool | Link |
|---|---|
| Visual Studio 2022 Community | https://visualstudio.microsoft.com/ (workload "Desktop C++") |
| Qt6 | https://www.qt.io/download-qt-installer (Qt 6.7 → MSVC 2019 64-bit) |
| CMake | https://cmake.org/download/ |
| Ninja | https://ninja-build.org/ |
| NSIS | https://nsis.sourceforge.io/ |

### MuPDF

Compila da sorgente con MSBuild:

```bat
git clone --recurse-submodules git://git.ghostscript.com/mupdf.git
cd mupdf
msbuild platform\win32\mupdf.sln /p:Configuration=Release /p:Platform=x64 /t:libmupdf /m
```

### Build

```bat
cmake -B build -G "Ninja" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DMUPDF_INCLUDE_DIRS="percorso\mupdf\include" ^
  -DMUPDF_LIBRARIES="percorso\mupdf\platform\win32\x64\Release\libmupdf.lib"

cmake --build build --parallel
```

### Deploy + Installer

```bat
mkdir deploy
copy build\sfogliaccio.exe deploy\
windeployqt --release --no-translations deploy\sfogliaccio.exe
makensis installer.nsi
```

---

## Struttura output finale

```
dist/
├── SfogliAccio-1.0.0-x86_64.AppImage   ← Linux (tutto in uno)
└── SfogliAccio-Setup.exe               ← Windows installer
```

### L'installer Windows include:
- Installazione in `C:\Program Files\SFOGLI-ACCIO\`
- Shortcut sul desktop e nel menu Start
- Associazione file `.pdf`
- Disinstallatore (visibile in Pannello di controllo → Programmi)
