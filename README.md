# VISTA PDF — C++/Qt6

Lettore PDF desktop nativo, veloce e gratuito.
**Stack:** Qt6 + MuPDF + CMake/Ninja

---

## Dipendenze

### Ubuntu / Debian
```bash
sudo apt install \
  qt6-base-dev libqt6concurrent6-dev qt6-base-private-dev \
  libmupdf-dev \
  cmake ninja-build \
  libfreetype-dev libharfbuzz-dev libjpeg-dev \
  libopenjp2-7-dev libjbig2dec0-dev libgumbo-dev
```

### Arch Linux
```bash
sudo pacman -S qt6-base mupdf-tools cmake ninja
```

### Fedora
```bash
sudo dnf install qt6-qtbase-devel mupdf-devel cmake ninja-build
```

---

## Build

```bash
git clone / unzip vista-cpp
cd vista-cpp
mkdir build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja
./vista                        # avvia
./vista /percorso/doc.pdf      # apri un file direttamente
```

### Debug build
```bash
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug
ninja
```

---

## Installazione di sistema (opzionale)

```bash
cd build
sudo cmake --install .
# Installa in /usr/local/bin/vista
# Installa il .desktop in /usr/local/share/applications/
```

---

## Struttura del progetto

```
vista-cpp/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── Theme.h            # Colori, font, stylesheet globale
│   ├── MainWindow.h       # Finestra principale
│   ├── PdfDocument.h      # Wrapper MuPDF (rendering, ricerca, bookmarks)
│   ├── PdfView.h          # Scroll area con pagine PDF (rendering asincrono)
│   ├── SidebarWidget.h    # Miniature, segnalibri, metadati
│   ├── SearchBar.h        # Barra di ricerca
│   ├── TitleBar.h         # Titlebar custom con controlli finestra
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
    └── vista.desktop
```

---

## Funzionalità

| Feature | Note |
|---|---|
| Rendering PDF vettoriale | MuPDF, rendering a qualsiasi DPI |
| Zoom libero | 10% – 600%, step 15% |
| Adatta larghezza / pagina | Fit automatico |
| Rotazione | 0° / 90° / 180° / 270° |
| Inversione colori | Lettura notturna, nessuna perdita qualità |
| Selezione testo | Layer testo trasparente su ogni pagina |
| Ricerca full-text | Evidenzia tutte le occorrenze, naviga con ↑↓ |
| Sidebar miniature | Async, clic per saltare a pagina |
| Segnalibri PDF | Outline nativo, tree gerarchico |
| Metadati | Titolo, autore, produttore, ecc. |
| Stampa | Via QPrinter nativo del sistema |
| Drag & drop | Trascina PDF sulla finestra |
| Apertura da CLI | `vista documento.pdf` |
| Memoria posizione | Ripristina dimensione finestra |
| Titlebar custom | Drag-to-move, massimizza con doppio clic |
| Schermo intero | F11 |

---

## Scorciatoie tastiera

| Tasto | Azione |
|---|---|
| `Ctrl+O` | Apri file |
| `Ctrl+F` | Cerca |
| `Ctrl+B` | Sidebar |
| `Ctrl+I` | Inverti colori |
| `Ctrl+=` | Zoom in |
| `Ctrl+-` | Zoom out |
| `Ctrl+0` | Zoom 100% |
| `Ctrl+Shift+W` | Adatta larghezza |
| `Ctrl+Shift+H` | Adatta pagina |
| `Ctrl+R` | Ruota orario |
| `Ctrl+Shift+R` | Ruota antiorario |
| `Ctrl+P` | Stampa |
| `←` / `→` | Pagina precedente / successiva |
| `Home` / `End` | Prima / ultima pagina |
| `F11` | Schermo intero |
| `F1` | Informazioni |

---

## Note tecniche

- Il rendering avviene su thread separati via **QtConcurrent** — la UI non si blocca mai
- MuPDF è significativamente più veloce di Poppler per rendering e ricerca
- Il layer testo è generato da MuPDF e sovrapposto trasparente ai canvas per abilitare la selezione
- La ricerca usa `fz_search_page_number` che restituisce quad di caratteri con coordinate precise

---

## Licenza

MIT — usa, modifica, distribuisci liberamente.
