# SFOGLI-ACCIO

> *Lettore PDF desktop nativo, veloce e gratuito. Fatto con ♥ da ACCIO.*

---

> [!IMPORTANT]
> **⚠️ Progetto generato interamente con l'ausilio dell'intelligenza artificiale**
>
> Questo progetto — inclusi tutti i file sorgente, la documentazione e questo README — è stato **ideato, progettato e scritto integralmente tramite AI** (Claude di Anthropic), con supervisione umana da parte dell'autore.
>
> Potrebbero essere presenti **bug, scelte architetturali non ottimali, o comportamenti inattesi**. Contributi, segnalazioni e correzioni sono benvenuti. 🙏

---

**SFOGLI-ACCIO** è un lettore PDF moderno costruito in C++ puro con Qt6 e MuPDF — lo stesso motore di rendering di Sumatra PDF. Niente Electron, niente browser, niente bloat.

**Stack:** C++20 · Qt6 · MuPDF · CMake · Ninja

---

## Download

| Piattaforma | File |
|---|---|
| 🪟 Windows | `SfogliAccio-Setup.exe` dalla pagina [Releases](../../releases) |
| 🐧 Linux | `SfogliAccio-x86_64.AppImage` dalla pagina [Releases](../../releases) |

---

## Funzionalità

| Feature | Note |
|---|---|
| Rendering PDF vettoriale | MuPDF + QGraphicsView, qualità perfetta a qualsiasi DPI e zoom |
| Tab multipli | Apri più PDF in tab separati, Ctrl+T / Ctrl+W, drag per riordinare |
| Zoom libero | 10% – 600%, Ctrl+Scroll, pinch trackpad, step 15% |
| Fit automatico | Si adatta alla larghezza alla prima apertura |
| Adatta larghezza / pagina | Fit automatico con un clic |
| Rotazione | 0° / 90° / 180° / 270° |
| Inversione colori | Lettura notturna, nessuna perdita qualità |
| Tema chiaro/scuro | Ctrl+Shift+T |
| Selezione testo | Precisa al pixel, doppio clic per parola |
| Copia testo | Ctrl+C |
| Evidenziatore | 6 colori: blu, giallo, verde, rosso, viola, rosa |
| Ricerca full-text | Asincrona con progressivo, evidenzia tutte le occorrenze |
| Modalità presentazione | F5 — fullscreen senza UI, Esc per uscire |
| Doppia pagina | Ctrl+D — due pagine affiancate stile libro |
| Cronologia navigazione | Alt+← / Alt+→ |
| Sidebar | Miniature, indice PDF, preferiti personali, metadati |
| Preferiti personali | Segnalibri per pagina salvati per ogni documento |
| PDF con password | Dialog automatico per PDF protetti |
| Stampa | Via QPrinter nativo del sistema |
| Drag & drop | Trascina PDF sulla finestra |
| Apertura da CLI | `sfogliaccio documento.pdf` / `sfogliaccio doc.pdf --page 42` |
| File recenti | Ultimi 10 file |
| Memoria sessione | Ripristina zoom e pagina per ogni documento |
| Ridimensionamento finestra | Resize nativo su tutti i bordi |

---

## Scorciatoie tastiera

| Tasto | Azione |
|---|---|
| `Ctrl+O` | Apri file |
| `Ctrl+T` | Nuovo tab |
| `Ctrl+W` | Chiudi tab |
| `Ctrl+1-9` | Passa al tab N |
| `Ctrl+F` | Cerca nel documento |
| `Ctrl+C` | Copia testo selezionato |
| `Ctrl+A` | Seleziona tutto (pagina corrente) |
| `Ctrl+M` | Aggiungi pagina ai preferiti |
| `Ctrl+B` | Mostra/nascondi sidebar |
| `Ctrl+I` | Inverti colori |
| `Ctrl+D` | Modalità doppia pagina |
| `Ctrl+Shift+T` | Tema chiaro/scuro |
| `Ctrl+=` | Zoom in |
| `Ctrl+-` | Zoom out |
| `Ctrl+0` | Zoom 100% |
| `Ctrl+Scroll` | Zoom con rotella mouse |
| `Ctrl+Shift+W` | Adatta larghezza |
| `Ctrl+Shift+H` | Adatta pagina |
| `Ctrl+R` | Ruota orario |
| `Ctrl+Shift+R` | Ruota antiorario |
| `Ctrl+P` | Stampa |
| `Alt+←` / `Alt+→` | Cronologia navigazione |
| `Ctrl+←` / `Ctrl+→` | Pagina precedente / successiva |
| `Ctrl+Home` / `Ctrl+End` | Prima / ultima pagina |
| `F5` | Modalità presentazione |
| `Esc` | Esci dalla presentazione |
| `F11` | Schermo intero |
| `F1` | Informazioni |

---

## Build da sorgente

### Dipendenze

**Arch Linux**
```bash
sudo pacman -S qt6-base mupdf-tools cmake ninja
```

**Ubuntu / Debian**
```bash
sudo apt install \
  qt6-base-dev libqt6concurrent6-dev \
  libmupdf-dev cmake ninja-build \
  libfreetype-dev libharfbuzz-dev libjpeg-dev \
  libopenjp2-7-dev libjbig2dec0-dev libgumbo-dev
```

**Fedora**
```bash
sudo dnf install qt6-qtbase-devel mupdf-devel cmake ninja-build
```

### Compila

```bash
git clone https://github.com/gitAccio/sfogliaccio.git
cd sfogliaccio
cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/sfogliaccio
```

### Apertura da riga di comando

```bash
sfogliaccio documento.pdf
sfogliaccio documento.pdf --page 42
sfogliaccio documento.pdf -p 42
```

### Installazione di sistema (Linux)

```bash
sudo cmake --install build
```

---

## Struttura del progetto

```
sfogliaccio/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── INSTALL.md
├── include/
│   ├── Theme.h            # Colori, font, stylesheet globale
│   ├── MainWindow.h       # Finestra principale con gestione tab
│   ├── PdfDocument.h      # Wrapper MuPDF thread-safe, ricerca asincrona
│   ├── PdfView.h          # QGraphicsView + QGraphicsScene lazy rendering
│   ├── SidebarWidget.h    # Miniature, indice, preferiti, metadati
│   ├── SearchBar.h        # Barra di ricerca con progress indicator
│   ├── TitleBar.h         # Titlebar con tab integrati
│   └── ZoomController.h   # Widget zoom
├── src/
│   └── *.cpp
└── resources/
    ├── resources.qrc
    ├── icon.png / icon.ico / app.rc
    └── sfogliaccio.desktop
```

---

## Note tecniche

- Architettura **QGraphicsView** — rendering lazy, zoom hardware-accelerated via trasformazione matriciale, gestione nativa di documenti con migliaia di pagine
- **MuPDF** — motore di rendering più veloce disponibile (stesso di Sumatra PDF)
- **Ricerca asincrona** — `QtConcurrent` in background, risultati progressivi senza bloccare l'UI
- **Lazy rendering** — solo le pagine visibili vengono renderizzate, buffer di ±2 pagine
- **Zoom trackpad** — pinch gesture via `QNativeGestureEvent`, anteprima visiva istantanea, re-render alla fine del gesto
- Compatibile con MuPDF 1.19+ (Ubuntu 22.04) e 1.24+ (Arch) via macro di compatibilità API

---

## Licenza

MIT License — Copyright (c) 2025 ACCIO

Vedi file [LICENSE](LICENSE) per il testo completo.
