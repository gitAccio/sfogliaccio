# SFOGLI-ACCIO

> *Lettore PDF desktop nativo, veloce e gratuito. Fatto con ♥ da ACCIO.*

---

> [!IMPORTANT]
> **⚠️ Progetto generato interamente con l'ausilio dell'intelligenza artificiale**
>
> Questo progetto — inclusi tutti i file sorgente, la documentazione e questo README — è stato **ideato, progettato e scritto integralmente tramite AI** (Claude di Anthropic), con supervisione umana da parte dell'autore.
>
> Ciò significa che potrebbero essere presenti **bug, scelte architetturali non ottimali, o comportamenti inattesi**. Il codice funziona ed è stato testato, ma non ha attraversato un processo di revisione tradizionale da parte di sviluppatori esperti.
>
> Contributi, segnalazioni e correzioni sono benvenuti. 🙏

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
| Rendering PDF vettoriale | MuPDF, qualità perfetta a qualsiasi DPI e zoom |
| Tab multipli | Apri più PDF in tab separati, Ctrl+T / Ctrl+W |
| Zoom libero | 10% – 600%, Ctrl+Scroll, step 15% |
| Adatta larghezza / pagina | Fit automatico con un clic |
| Rotazione | 0° / 90° / 180° / 270° |
| Inversione colori | Lettura notturna, nessuna perdita qualità |
| Selezione testo | Layer testo trasparente su ogni pagina, riconoscimento geometrico delle righe |
| Copia testo | Ctrl+C, doppio clic per selezionare parola |
| Evidenziatore | 6 colori: blu, giallo, verde, rosso, viola, rosa |
| Ricerca full-text | Evidenzia tutte le occorrenze, naviga con Invio, scroll animato |
| Sidebar | Miniature async, segnalibri PDF, metadati documento |
| Stampa | Via QPrinter nativo del sistema |
| Drag & drop | Trascina PDF sulla finestra |
| Apertura da CLI | `sfogliaccio documento.pdf` |
| File recenti | Ultimi 10 file, con pulizia automatica |
| Ridimensionamento finestra | Resize nativo su tutti i bordi |
| Titlebar custom | Drag-to-move, doppio clic per massimizzare |
| Memoria finestra | Ripristina posizione e dimensione all'avvio |
| Schermo intero | F11 |

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
| `Ctrl+B` | Mostra/nascondi sidebar |
| `Ctrl+I` | Inverti colori |
| `Ctrl+=` | Zoom in |
| `Ctrl+-` | Zoom out |
| `Ctrl+0` | Zoom 100% |
| `Ctrl+Scroll` | Zoom con rotella mouse |
| `Ctrl+Shift+W` | Adatta larghezza |
| `Ctrl+Shift+H` | Adatta pagina |
| `Ctrl+R` | Ruota orario |
| `Ctrl+Shift+R` | Ruota antiorario |
| `Ctrl+P` | Stampa |
| `Ctrl+←` / `Ctrl+→` | Pagina precedente / successiva |
| `Ctrl+Home` / `Ctrl+End` | Prima / ultima pagina |
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

### Installazione di sistema (Linux)

```bash
sudo cmake --install build
# Installa in /usr/local/bin/sfogliaccio
# Aggiunge sfogliaccio.desktop al menu applicazioni
```

---

## Struttura del progetto

```
sfogliaccio/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── include/
│   ├── Theme.h            # Colori, font, stylesheet globale
│   ├── MainWindow.h       # Finestra principale con gestione tab
│   ├── PdfDocument.h      # Wrapper MuPDF thread-safe
│   ├── PdfView.h          # Scroll area + layer testo interattivo
│   ├── SidebarWidget.h    # Miniature, segnalibri, metadati
│   ├── SearchBar.h        # Barra di ricerca
│   ├── TitleBar.h         # Titlebar custom
│   └── ZoomController.h   # Widget zoom
├── src/
│   ├── main.cpp
│   ├── MainWindow.cpp
│   ├── PdfDocument.cpp
│   ├── PdfView.cpp
│   ├── SidebarWidget.cpp
│   ├── SearchBar.cpp
│   ├── TitleBar.cpp
│   └── ZoomController.cpp
└── resources/
    ├── resources.qrc
    ├── icon.png
    ├── icon.ico
    ├── app.rc
    └── sfogliaccio.desktop
```

---

## Note tecniche

- Il rendering avviene su thread separati via **QtConcurrent** — la UI non si blocca mai
- **MuPDF** è significativamente più veloce di Poppler per rendering e ricerca
- Il layer testo usa le bounding box reali dei caratteri estratte da `fz_stext_page` — la selezione è precisa al pixel
- La ricerca usa coordinate PDF native poi scalate allo zoom corrente, con scroll animato (180ms easing OutCubic) alla corrispondenza attiva
- Su Windows il resize usa `WM_NCHITTEST` nativo per comportamento identico alle finestre di sistema
- Compatibile con MuPDF 1.19+ (Ubuntu 22.04) e 1.24+ (Arch) tramite macro di compatibilità API

---

## Licenza

MIT License — Copyright (c) 2025 ACCIO

Vedi file [LICENSE](LICENSE) per il testo completo.
