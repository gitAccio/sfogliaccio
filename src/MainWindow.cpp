#include "MainWindow.h"
#include "Theme.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QPainter>
#include <QSettings>
#include <QScreen>
#include <QCloseEvent>
#include <QTimer>
#include <QFrame>
#include <QPushButton>
#include <QClipboard>
#include <QShortcut>
#include <QWindow>
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <windowsx.h>
#endif

// ─── Helpers ──────────────────────────────────────────────────────────────────
static QPushButton *makeTbBtn(const QString &text, const QString &tip,
                               QWidget *parent, bool accent = false)
{
    auto *btn = new QPushButton(text, parent);
    btn->setToolTip(tip);
    btn->setFixedHeight(34);
    if (accent) {
        btn->setStyleSheet(QString(
            "QPushButton{"
            " background:%1;color:#111;border:1px solid %1;"
            " border-radius:4px;font-weight:700;font-size:13px;"
            " padding:0 14px;letter-spacing:1px;"
            "}"
            "QPushButton:hover{background:#cfe030;border-color:#cfe030;}")
            .arg(Theme::ACCENT));
    } else {
        btn->setStyleSheet(QString(
            "QPushButton{"
            " background:transparent;color:%1;"
            " border:1px solid transparent;border-radius:4px;"
            " font-size:13px;padding:0 10px;"
            "}"
            "QPushButton:hover{background:%2;border-color:%3;color:%4;}"
            "QPushButton:checked{color:%5;border-color:%5;background:%6;}")
            .arg(Theme::TEXT_DIM).arg(Theme::SURFACE3)
            .arg(Theme::BORDER2).arg(Theme::TEXT)
            .arg(Theme::ACCENT).arg(Theme::ACCENT_DIM));
    }
    return btn;
}

static QFrame *makeSep(QWidget *parent)
{
    auto *f = new QFrame(parent);
    f->setFrameShape(QFrame::VLine);
    f->setFixedSize(1, 24);
    f->setStyleSheet(QString("background:%1;").arg(Theme::BORDER));
    return f;
}

// ═══════════════════════════════════════════════════════════════════════════
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("SFOGLI-ACCIO");
    setMinimumSize(900, 600);
    setAcceptDrops(true);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    // Abilita ridimensionamento nativo su Windows mantenendo frameless
    setAttribute(Qt::WA_TranslucentBackground, false);
    // Installa event filter per resize sui bordi
    installEventFilter(this);

    QSettings s("SfogliAccio", "SfogliAccio");
    m_recentFiles = s.value("recentFiles").toStringList();

    buildUI();
    buildShortcuts();
    buildStatusBar();

    restoreGeometry(s.value("geometry").toByteArray());
    if (!s.contains("geometry")) {
        resize(1380, 860);
        if (auto *sc = QGuiApplication::primaryScreen())
            move((sc->geometry().width()-1380)/2, (sc->geometry().height()-860)/2);
    }
}

// ─── buildUI ─────────────────────────────────────────────────────────────────
void MainWindow::buildUI()
{
    auto *central = new QWidget(this);
    central->setStyleSheet(QString("background:%1;").arg(Theme::BG));
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // TitleBar
    m_titleBar = new TitleBar(this);
    connect(m_titleBar, &TitleBar::minimizeRequested, this, &QMainWindow::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeRequested, this, [this]{
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(m_titleBar, &TitleBar::closeRequested, this, &QMainWindow::close);
    root->addWidget(m_titleBar);

    // Tab bar row: tabbar + "+" button side by side
    buildTabBar();
    {
        auto *tabRow = new QWidget(this);
        tabRow->setFixedHeight(Theme::TABBAR_H);
        tabRow->setStyleSheet(QString("background:%1;border-bottom:1px solid %2;")
            .arg(Theme::SURFACE).arg(Theme::BORDER));
        auto *tabRowL = new QHBoxLayout(tabRow);
        tabRowL->setContentsMargins(0,0,0,0);
        tabRowL->setSpacing(0);
        tabRowL->addWidget(m_tabBar, 1);
        m_newTabBtn->setParent(tabRow);
        tabRowL->addWidget(m_newTabBtn, 0);
        root->addWidget(tabRow);
    }

    // Toolbar
    buildToolbar();
    root->addWidget(m_toolbar);

    // Search bar
    m_searchBar = new SearchBar(this);
    m_searchBar->setVisible(false);
    connect(m_searchBar, &SearchBar::searchRequested, this, &MainWindow::onSearchRequested);
    connect(m_searchBar, &SearchBar::navigateNext,    this, &MainWindow::onSearchNext);
    connect(m_searchBar, &SearchBar::navigatePrev,    this, &MainWindow::onSearchPrev);
    connect(m_searchBar, &SearchBar::closed,          this, &MainWindow::onSearchClose);
    root->addWidget(m_searchBar);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(2);
    m_progressBar->setRange(0,100);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(QString(
        "QProgressBar{background:transparent;border:none;}"
        "QProgressBar::chunk{background:%1;}").arg(Theme::ACCENT));
    m_progressBar->setVisible(false);
    root->addWidget(m_progressBar);

    // Tab content stack (one splitter per tab + welcome screen)
    m_tabStack = new QStackedWidget(this);

    // Welcome screen (shown when no tabs)
    m_welcome = new QWidget(m_tabStack);
    m_welcome->setStyleSheet(QString("background:%1;").arg(Theme::BG));
    {
        auto *wL = new QVBoxLayout(m_welcome);
        wL->setAlignment(Qt::AlignCenter);
        wL->setSpacing(20);

        auto *ico = new QLabel("PDF", m_welcome);
        ico->setAlignment(Qt::AlignCenter);
        ico->setFixedSize(90,90);
        ico->setStyleSheet(QString(
            "font-size:16px;font-weight:800;"
            "color:%1;border:1px solid %2;border-radius:8px;"
            "background:%3;letter-spacing:3px;")
            .arg(Theme::ACCENT).arg(Theme::BORDER2).arg(Theme::SURFACE2));

        auto *titleW = new QWidget(m_welcome);
        auto *titleL = new QHBoxLayout(titleW);
        titleL->setContentsMargins(0,0,0,0);
        titleL->setSpacing(0);
        titleL->setAlignment(Qt::AlignCenter);
        auto *t1 = new QLabel("SFOGLI-", titleW);
        auto *t2 = new QLabel("ACCIO",   titleW);
        QString ts = "font-size:42px;font-weight:800;letter-spacing:4px;background:transparent;";
        t1->setStyleSheet(ts + QString("color:%1;").arg(Theme::TEXT));
        t2->setStyleSheet(ts + QString("color:%1;").arg(Theme::ACCENT));
        titleL->addWidget(t1); titleL->addWidget(t2);

        auto *sub = new QLabel("Trascina un PDF qui, oppure clicca  Apri", m_welcome);
        sub->setAlignment(Qt::AlignCenter);
        sub->setStyleSheet(QString("color:%1;font-size:14px;").arg(Theme::TEXT_DIM));

        auto *btn = new QPushButton("  Apri PDF  ", m_welcome);
        btn->setFixedHeight(44);
        btn->setStyleSheet(QString(
            "QPushButton{background:%1;color:#111;border:none;border-radius:4px;"
            "font-size:15px;font-weight:700;letter-spacing:2px;padding:0 36px;}"
            "QPushButton:hover{background:#cfe030;}").arg(Theme::ACCENT));
        connect(btn, &QPushButton::clicked, this, &MainWindow::onOpenFile);

        auto *scW = new QWidget(m_welcome);
        auto *scL = new QHBoxLayout(scW);
        scL->setSpacing(24);
        for (const QString &s : {"Ctrl+O  Apri","Ctrl+T  Nuovo tab","Ctrl+F  Cerca",
                                  "Ctrl+B  Sidebar","Ctrl+I  Inverti","Ctrl+Scroll  Zoom"}) {
            auto *l = new QLabel(s, scW);
            l->setStyleSheet(QString("color:%1;font-size:12px;").arg(Theme::TEXT_DIMMER));
            scL->addWidget(l);
        }

        wL->addStretch();
        wL->addWidget(ico,    0, Qt::AlignCenter);
        wL->addWidget(titleW, 0, Qt::AlignCenter);
        wL->addWidget(sub,    0, Qt::AlignCenter);
        wL->addWidget(btn,    0, Qt::AlignCenter);
        wL->addWidget(scW,    0, Qt::AlignCenter);
        wL->addStretch();
    }
    m_tabStack->addWidget(m_welcome);
    m_tabStack->setCurrentWidget(m_welcome);

    root->addWidget(m_tabStack, 1);

    // Build recent-files menu container
    menuBar()->hide();
    auto *file   = menuBar()->addMenu("File");
    m_recentMenu = file->addMenu("Apri recenti");
    rebuildRecentMenu();
}

void MainWindow::buildTabBar()
{
    m_tabBar = new QTabBar(this);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setFixedHeight(Theme::TABBAR_H);
    m_tabBar->setStyleSheet(QString(R"(
        QTabBar { background:%1; }
        QTabBar::tab {
            background:%1; color:%3;
            padding:0 18px; height:%4px;
            min-width:120px; max-width:220px;
            border-right:1px solid %2; font-size:12px;
        }
        QTabBar::tab:selected  { background:%5; color:%6; border-bottom:2px solid %6; }
        QTabBar::tab:hover:!selected { background:%7; color:%8; }
    )").arg(Theme::SURFACE).arg(Theme::BORDER)
       .arg(Theme::TEXT_DIM).arg(Theme::TABBAR_H)
       .arg(Theme::SURFACE2).arg(Theme::ACCENT)
       .arg(Theme::SURFACE3).arg(Theme::TEXT));

    m_newTabBtn = new QPushButton("+", this);
    m_newTabBtn->setFixedSize(Theme::TABBAR_H, Theme::TABBAR_H);
    m_newTabBtn->setToolTip("Apri nuovo tab (Ctrl+T)");
    m_newTabBtn->setStyleSheet(QString(
        "QPushButton{background:transparent;color:%1;border:none;font-size:20px;font-weight:300;}"
        "QPushButton:hover{background:%2;color:%3;}")
        .arg(Theme::TEXT_DIM).arg(Theme::SURFACE3).arg(Theme::ACCENT));
    connect(m_newTabBtn, &QPushButton::clicked, this, &MainWindow::onNewTab);

    connect(m_tabBar, &QTabBar::currentChanged,    this, &MainWindow::onTabChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(m_tabBar, &QTabBar::tabMoved, this, [this](int from, int to){
        m_tabs.move(from, to);
    });
}

void MainWindow::buildToolbar()
{
    m_toolbar = new QWidget(this);
    m_toolbar->setFixedHeight(Theme::TOOLBAR_H);
    m_toolbar->setStyleSheet(QString(
        "QWidget{background:%1;border-bottom:1px solid %2;}")
        .arg(Theme::SURFACE).arg(Theme::BORDER));

    auto *tbL = new QHBoxLayout(m_toolbar);
    tbL->setContentsMargins(10,0,10,0);
    tbL->setSpacing(4);

    auto *openBtn = makeTbBtn("Apri", "Apri PDF (Ctrl+O)", m_toolbar, true);
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    tbL->addWidget(openBtn);
    tbL->addWidget(makeSep(m_toolbar));

    m_sidebarBtn = makeTbBtn("≡", "Sidebar (Ctrl+B)", m_toolbar);
    m_sidebarBtn->setCheckable(true);
    m_sidebarBtn->setFixedWidth(40);
    connect(m_sidebarBtn, &QPushButton::clicked, this, &MainWindow::onToggleSidebar);
    tbL->addWidget(m_sidebarBtn);
    tbL->addWidget(makeSep(m_toolbar));

    // Nav group
    m_navGroup = new QWidget(m_toolbar);
    auto *navL = new QHBoxLayout(m_navGroup);
    navL->setContentsMargins(0,0,0,0);
    navL->setSpacing(3);

    auto *firstBtn = makeTbBtn("⏮", "Prima pagina", m_navGroup);
    auto *prevBtn  = makeTbBtn("◀", "Pagina precedente (←)", m_navGroup);
    firstBtn->setFixedWidth(36); prevBtn->setFixedWidth(36);

    m_pageInput = new QLineEdit(m_navGroup);
    m_pageInput->setFixedSize(44, 32);
    m_pageInput->setAlignment(Qt::AlignCenter);
    m_pageInput->setStyleSheet(QString(
        "QLineEdit{background:%1;border:1px solid %2;border-radius:4px;"
        "color:%3;font-size:13px;}"
        "QLineEdit:focus{border-color:%4;}")
        .arg(Theme::SURFACE3).arg(Theme::BORDER)
        .arg(Theme::TEXT).arg(Theme::ACCENT));

    m_pageTotalLbl = new QLabel("/ —", m_navGroup);
    m_pageTotalLbl->setStyleSheet(
        QString("color:%1;font-size:13px;").arg(Theme::TEXT_DIM));

    auto *nextBtn = makeTbBtn("▶", "Pagina successiva (→)", m_navGroup);
    auto *lastBtn = makeTbBtn("⏭", "Ultima pagina", m_navGroup);
    nextBtn->setFixedWidth(36); lastBtn->setFixedWidth(36);

    navL->addWidget(firstBtn); navL->addWidget(prevBtn);
    navL->addWidget(m_pageInput); navL->addWidget(m_pageTotalLbl);
    navL->addWidget(nextBtn); navL->addWidget(lastBtn);
    m_navGroup->setVisible(false);

    connect(firstBtn, &QPushButton::clicked, this, [this]{ if(auto*t=currentTab()) t->view->scrollToPage(0); });
    connect(prevBtn,  &QPushButton::clicked, this, [this]{ if(auto*t=currentTab()) t->view->scrollToPage(t->view->currentPage()-1); });
    connect(nextBtn,  &QPushButton::clicked, this, [this]{ if(auto*t=currentTab()) t->view->scrollToPage(t->view->currentPage()+1); });
    connect(lastBtn,  &QPushButton::clicked, this, [this]{ if(auto*t=currentTab()) t->view->scrollToPage(t->view->pageCount()-1); });
    connect(m_pageInput, &QLineEdit::returnPressed, this, &MainWindow::onPageInputReturn);

    tbL->addWidget(m_navGroup);
    tbL->addWidget(makeSep(m_toolbar));

    m_zoom = new ZoomController(m_toolbar);
    connect(m_zoom, &ZoomController::zoomChanged, this, [this](float z){
        if (auto *t = currentTab()) t->view->setZoom(z);
        m_sbZoom->setText(QString("%1%").arg(qRound(z*100)));
    });
    tbL->addWidget(m_zoom);

    auto *fitWBtn = makeTbBtn("↔ Larghezza", "Adatta larghezza (Ctrl+Shift+W)", m_toolbar);
    auto *fitHBtn = makeTbBtn("↕ Pagina",    "Adatta pagina (Ctrl+Shift+H)",    m_toolbar);
    connect(fitWBtn, &QPushButton::clicked, this, &MainWindow::onFitWidth);
    connect(fitHBtn, &QPushButton::clicked, this, &MainWindow::onFitPage);
    tbL->addWidget(fitWBtn);
    tbL->addWidget(fitHBtn);
    tbL->addWidget(makeSep(m_toolbar));

    auto *rotCWBtn  = makeTbBtn("↻", "Ruota orario (Ctrl+R)", m_toolbar);
    auto *rotCCWBtn = makeTbBtn("↺", "Ruota antiorario",       m_toolbar);
    rotCWBtn->setFixedWidth(38); rotCCWBtn->setFixedWidth(38);
    connect(rotCWBtn,  &QPushButton::clicked, this, &MainWindow::onRotateCW);
    connect(rotCCWBtn, &QPushButton::clicked, this, &MainWindow::onRotateCCW);
    tbL->addWidget(rotCWBtn);
    tbL->addWidget(rotCCWBtn);

    m_invertBtn = makeTbBtn("◑ Inverti", "Inverti colori (Ctrl+I)", m_toolbar);
    m_invertBtn->setCheckable(true);
    connect(m_invertBtn, &QPushButton::clicked, this, &MainWindow::onToggleInvert);
    tbL->addWidget(m_invertBtn);

    // Evidenziatore testo
    auto *hlMenu = new QMenu(m_toolbar);
    struct HLColor { QString name; QColor color; };
    const QList<HLColor> hlColors = {
        {"🔵 Blu",       QColor(77,  168, 255, 110)},
        {"🟡 Giallo",    QColor(255, 230,  50, 130)},
        {"🟢 Verde",     QColor( 80, 220, 100, 110)},
        {"🔴 Rosso",     QColor(255,  80,  80, 110)},
        {"🟣 Viola",     QColor(180,  80, 255, 110)},
        {"🩷 Rosa",      QColor(255, 120, 180, 110)},
    };
    for (const auto &hc : hlColors) {
        auto *act = hlMenu->addAction(hc.name);
        connect(act, &QAction::triggered, this, [this, hc]{
            if (auto *t = currentTab()) t->view->setSelectionColor(hc.color);
            setStatusMessage("Colore evidenziazione: " + hc.name.mid(3));
        });
    }
    hlMenu->addSeparator();
    auto *resetAct = hlMenu->addAction("↩ Ripristina");
    connect(resetAct, &QAction::triggered, this, [this]{
        if (auto *t = currentTab()) t->view->setSelectionColor(QColor(77,168,255,90));
    });
    auto *hlBtn = makeTbBtn("🖊 Evidenzia", "Colore selezione testo", m_toolbar);
    hlBtn->setMenu(hlMenu);
    tbL->addWidget(hlBtn);
    tbL->addWidget(makeSep(m_toolbar));

    tbL->addStretch();

    m_searchOpenBtn = makeTbBtn("🔍 Cerca", "Cerca (Ctrl+F)", m_toolbar);
    connect(m_searchOpenBtn, &QPushButton::clicked, this, &MainWindow::onSearch);
    tbL->addWidget(m_searchOpenBtn);

    auto *printBtn = makeTbBtn("🖨 Stampa", "Stampa (Ctrl+P)", m_toolbar);
    connect(printBtn, &QPushButton::clicked, this, &MainWindow::onPrint);
    tbL->addWidget(printBtn);
}

void MainWindow::buildShortcuts()
{
    auto sc = [&](const QKeySequence &ks, auto slot) {
        auto *s = new QShortcut(ks, this);
        s->setContext(Qt::ApplicationShortcut);
        connect(s, &QShortcut::activated, this, slot);
    };

    sc(QKeySequence::Open,                 &MainWindow::onOpenFile);
    sc({Qt::CTRL|Qt::Key_T},               &MainWindow::onNewTab);
    sc({Qt::CTRL|Qt::Key_W},               [this]{ onTabCloseRequested(m_currentTabIdx); });
    sc(QKeySequence::Print,                &MainWindow::onPrint);
    sc({Qt::CTRL|Qt::Key_Q},               []{ qApp->quit(); });

    sc({Qt::CTRL|Qt::Key_Equal},           [this]{ m_zoom->zoomIn(); });
    sc({Qt::CTRL|Qt::Key_Plus},            [this]{ m_zoom->zoomIn(); });
    sc({Qt::CTRL|Qt::Key_Minus},           [this]{ m_zoom->zoomOut(); });
    sc({Qt::CTRL|Qt::Key_0},               [this]{ m_zoom->resetZoom(); });
    sc({Qt::CTRL|Qt::SHIFT|Qt::Key_W},     &MainWindow::onFitWidth);
    sc({Qt::CTRL|Qt::SHIFT|Qt::Key_H},     &MainWindow::onFitPage);
    sc({Qt::CTRL|Qt::Key_R},               &MainWindow::onRotateCW);
    sc({Qt::CTRL|Qt::SHIFT|Qt::Key_R},     &MainWindow::onRotateCCW);
    sc({Qt::CTRL|Qt::Key_I},               &MainWindow::onToggleInvert);
    sc({Qt::CTRL|Qt::Key_B},               &MainWindow::onToggleSidebar);
    sc(QKeySequence::FullScreen,           [this]{ isFullScreen()?showNormal():showFullScreen(); });
    sc({Qt::Key_F11},                      [this]{ isFullScreen()?showNormal():showFullScreen(); });

    sc({Qt::CTRL|Qt::Key_Left},            [this]{ if(auto*t=currentTab()) t->view->scrollToPage(t->view->currentPage()-1); });
    sc({Qt::CTRL|Qt::Key_Right},           [this]{ if(auto*t=currentTab()) t->view->scrollToPage(t->view->currentPage()+1); });
    sc({Qt::CTRL|Qt::Key_Home},            [this]{ if(auto*t=currentTab()) t->view->scrollToPage(0); });
    sc({Qt::CTRL|Qt::Key_End},             [this]{ if(auto*t=currentTab()) t->view->scrollToPage(t->view->pageCount()-1); });

    sc(QKeySequence::Find,                 &MainWindow::onSearch);
    sc(QKeySequence::Copy,                 &MainWindow::onCopyText);
    sc(QKeySequence::SelectAll,            [this]{ if(auto*t=currentTab()) t->view->selectAll(); });
    sc({Qt::Key_F1},                       &MainWindow::onAbout);

    // Switch tabs with Ctrl+1..9
    for (int i = 0; i < 9; ++i) {
        int idx = i;
        sc({Qt::CTRL | Qt::Key(Qt::Key_1 + i)}, [this, idx]{
            if (idx < m_tabs.size()) m_tabBar->setCurrentIndex(idx);
        });
    }
}

void MainWindow::buildStatusBar()
{
    auto *sb = statusBar();
    sb->setFixedHeight(Theme::STATUSBAR_H);
    sb->setStyleSheet(QString(
        "QStatusBar{background:%1;border-top:1px solid %2;font-size:12px;}"
        "QStatusBar::item{border:none;}")
        .arg(Theme::SURFACE).arg(Theme::BORDER));

    auto dot = [&]()->QLabel*{
        auto *l=new QLabel("·");
        l->setStyleSheet(QString("color:%1;").arg(Theme::BORDER2));
        return l;
    };

    m_sbFile   = new QLabel("Nessun file");
    m_sbSize   = new QLabel();
    m_sbPage   = new QLabel();
    m_sbZoom   = new QLabel("100%");
    m_sbStatus = new QLabel("Pronto");

    m_sbFile->setStyleSheet(QString("color:%1;font-size:12px;").arg(Theme::ACCENT));
    m_sbStatus->setStyleSheet(QString("color:%1;font-size:12px;").arg(Theme::TEXT_DIMMER));

    sb->addWidget(m_sbFile);
    sb->addWidget(dot());
    sb->addWidget(m_sbSize);
    sb->addPermanentWidget(m_sbPage);
    sb->addPermanentWidget(dot());
    sb->addPermanentWidget(m_sbZoom);
    sb->addPermanentWidget(dot());
    sb->addPermanentWidget(m_sbStatus);
}

// ─── Tab management ───────────────────────────────────────────────────────────
PdfTab *MainWindow::currentTab()
{
    if (m_currentTabIdx < 0 || m_currentTabIdx >= m_tabs.size()) return nullptr;
    return m_tabs[m_currentTabIdx];
}

PdfTab *MainWindow::currentTab() const
{
    if (m_currentTabIdx < 0 || m_currentTabIdx >= m_tabs.size()) return nullptr;
    return m_tabs[m_currentTabIdx];
}

int MainWindow::tabIndexForPath(const QString &path) const
{
    for (int i = 0; i < m_tabs.size(); ++i)
        if (m_tabs[i]->path == path) return i;
    return -1;
}

void MainWindow::onNewTab()
{
    onOpenFile();
}

void MainWindow::onTabChanged(int index)
{
    if (index < 0 || index >= m_tabs.size()) return;
    m_currentTabIdx = index;
    switchToTab(index);
}

void MainWindow::onTabCloseRequested(int index)
{
    if (index < 0 || index >= m_tabs.size()) return;
    closeTab(index);
}

void MainWindow::switchToTab(int index)
{
    auto *tab = m_tabs[index];

    m_tabStack->setCurrentWidget(tab->splitter);

    // Restore zoom
    m_zoom->setZoom(tab->zoom);
    m_sbZoom->setText(QString("%1%").arg(qRound(tab->zoom*100)));

    // Update invert button
    m_invertBtn->setChecked(tab->view->inverted());

    // Update sidebar button
    m_sidebarBtn->setChecked(m_sidebarOpen);

    // Update nav controls
    int pages = tab->doc->pageCount();
    if (pages > 0) {
        m_navGroup->setVisible(true);
        m_pageTotalLbl->setText(QString("/ %1").arg(pages));
        m_pageInput->setText(QString::number(tab->view->currentPage()+1));
        m_sbPage->setText(QString("Pag. %1 / %2").arg(tab->view->currentPage()+1).arg(pages));
    } else {
        m_navGroup->setVisible(false);
    }

    // Status bar
    QFileInfo fi(tab->path);
    m_sbFile->setText(tab->title);
    m_sbSize->setText(fi.exists() ? formatFileSize(fi.size()) : "");

    setWindowTitle("SFOGLI-ACCIO — " + tab->title);
    m_titleBar->setFileName(tab->title);

    setStatusMessage(QString("%1 pagine").arg(pages));
}

void MainWindow::closeTab(int index)
{
    if (m_tabs.isEmpty()) return;

    auto *tab = m_tabs.takeAt(index);
    m_tabBar->removeTab(index);
    m_tabStack->removeWidget(tab->splitter);
    tab->splitter->deleteLater();
    tab->doc->deleteLater();
    delete tab;

    if (m_tabs.isEmpty()) {
        m_currentTabIdx = -1;
        m_tabStack->setCurrentWidget(m_welcome);
        m_navGroup->setVisible(false);
        m_sbFile->setText("Nessun file");
        m_sbSize->setText("");
        m_sbPage->setText("");
        setWindowTitle("SFOGLI-ACCIO");
        m_titleBar->setFileName("");
    } else {
        int newIdx = qMin(index, m_tabs.size()-1);
        m_currentTabIdx = newIdx;
        m_tabBar->setCurrentIndex(newIdx);
        switchToTab(newIdx);
    }
}

// ─── openFile ─────────────────────────────────────────────────────────────────
void MainWindow::onOpenFile()
{
    QString startDir;
    if (auto *t = currentTab()) startDir = QFileInfo(t->path).dir().absolutePath();

    QString path = QFileDialog::getOpenFileName(
        this, "Apri PDF", startDir,
        "Documenti PDF (*.pdf);;Tutti i file (*)");
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::openFile(const QString &path)
{
    // If already open in a tab, just switch to it
    int existing = tabIndexForPath(path);
    if (existing >= 0) { m_tabBar->setCurrentIndex(existing); return; }

    m_progressBar->setValue(10);
    m_progressBar->setVisible(true);
    setStatusMessage("Apertura...");

    // Create new tab
    auto *tab = new PdfTab();
    tab->path = path;
    tab->doc  = new PdfDocument(this);

    connect(tab->doc, &PdfDocument::loadError, this,
            [this](const QString &msg){ QMessageBox::critical(this, "Errore", msg); });

    if (!tab->doc->load(path)) {
        delete tab->doc; delete tab;
        m_progressBar->setVisible(false);
        return;
    }

    QFileInfo fi(path);
    tab->title = fi.fileName();
    tab->zoom  = 1.0f;

    // Build splitter (sidebar + view)
    tab->splitter = new QSplitter(Qt::Horizontal, nullptr);
    tab->splitter->setHandleWidth(1);
    tab->splitter->setStyleSheet(
        QString("QSplitter::handle{background:%1;}").arg(Theme::BORDER));

    tab->sidebar = new SidebarWidget(tab->splitter);
    connect(tab->sidebar, &SidebarWidget::pageRequested, this, [tab](int idx){
        tab->view->scrollToPage(idx);
    });

    tab->view = new PdfView(tab->splitter);
    connect(tab->view, &PdfView::currentPageChanged, this, &MainWindow::onCurrentPageChanged);
    connect(tab->view, &PdfView::renderProgress,     this, &MainWindow::onRenderProgress);
    connect(tab->view, &PdfView::zoomChangeRequested,this, &MainWindow::onZoomChangeRequested);

    tab->splitter->addWidget(tab->sidebar);
    tab->splitter->addWidget(tab->view);
    tab->splitter->setCollapsible(0, true);
    tab->splitter->setCollapsible(1, false);

    // Sidebar open state
    int sw = m_sidebarOpen ? Theme::SIDEBAR_W : 0;
    tab->splitter->setSizes({sw, tab->splitter->width()-sw});

    m_tabStack->addWidget(tab->splitter);
    m_tabs.append(tab);

    // Add tab to bar
    int tabIdx = m_tabBar->addTab(tab->title);
    m_tabBar->setTabToolTip(tabIdx, path);

    // Load content
    tab->view->setDocument(tab->doc);
    tab->sidebar->setDocument(tab->doc, tab->doc->pageCount());

    m_progressBar->setValue(60);
    addRecentFile(path);

    // Switch to new tab
    m_tabBar->setCurrentIndex(tabIdx);
    // onTabChanged fires and calls switchToTab

    QTimer::singleShot(600, this, [this]{ m_progressBar->setVisible(false); });
}

// ─── Zoom ─────────────────────────────────────────────────────────────────────
void MainWindow::onZoomChangeRequested(float zoom)
{
    if (auto *t = currentTab()) t->zoom = zoom;
    m_zoom->setZoom(zoom);
}

void MainWindow::onFitWidth()
{
    auto *t = currentTab(); if (!t) return;
    int avail = t->view->viewport()->width() - 48;
    PageInfo pi = t->doc->pageInfo(0);
    if (pi.sizePt.width() > 0 && avail > 0)
        m_zoom->setZoom((float)avail / (float)pi.sizePt.width());
}

void MainWindow::onFitPage()
{
    auto *t = currentTab(); if (!t) return;
    int aw = t->view->viewport()->width()  - 48;
    int ah = t->view->viewport()->height() - 48;
    PageInfo pi = t->doc->pageInfo(0);
    if (pi.sizePt.width() > 0 && pi.sizePt.height() > 0)
        m_zoom->setZoom(qMin((float)aw/(float)pi.sizePt.width(),
                             (float)ah/(float)pi.sizePt.height()));
}

void MainWindow::onRotateCW()  { if(auto*t=currentTab()) t->view->setRotation(t->view->rotation()+90); }
void MainWindow::onRotateCCW() { if(auto*t=currentTab()) t->view->setRotation(t->view->rotation()-90); }

void MainWindow::onToggleInvert()
{
    auto *t = currentTab(); if (!t) return;
    bool inv = !t->view->inverted();
    t->view->setInverted(inv);
    m_invertBtn->setChecked(inv);
    setStatusMessage(inv ? "Inversione colori attiva" : "Colori normali");
}

void MainWindow::onToggleSidebar()
{
    m_sidebarOpen = !m_sidebarOpen;
    m_sidebarBtn->setChecked(m_sidebarOpen);
    int sw = m_sidebarOpen ? Theme::SIDEBAR_W : 0;
    // Apply to current tab's splitter
    for (auto *t : m_tabs) {
        t->splitter->setSizes({sw, t->splitter->width()-sw});
    }
}

// ─── Search ───────────────────────────────────────────────────────────────────
void MainWindow::onSearch()
{
    m_searchBar->setVisible(true);
    m_searchOpenBtn->setVisible(false);
    m_searchBar->focusInput();
}

void MainWindow::onSearchRequested(const QString &q)
{
    auto *t = currentTab(); if (!t) return;
    if (q.length() < 2) {
        t->view->clearSearchMatches();
        m_searchBar->clearInfo();
        m_searchMatchTotal = 0;
        return;
    }
    setStatusMessage("Ricerca...");
    auto matches = t->doc->search(q);
    t->view->setSearchMatches(matches);
    m_searchMatchTotal = t->view->totalMatchCount();
    m_searchMatchIdx   = 0;
    m_searchBar->setMatchInfo(m_searchMatchTotal > 0 ? 1 : 0, m_searchMatchTotal);
    if (m_searchMatchTotal > 0) {
        t->view->jumpToMatch(0);
        setStatusMessage(QString("%1 risultati").arg(m_searchMatchTotal));
    } else {
        setStatusMessage("Nessun risultato");
    }
}

void MainWindow::onSearchNext()
{
    auto *t = currentTab(); if (!t || !m_searchMatchTotal) return;
    m_searchMatchIdx = (m_searchMatchIdx+1) % m_searchMatchTotal;
    t->view->jumpToMatch(m_searchMatchIdx);
    m_searchBar->setMatchInfo(m_searchMatchIdx+1, m_searchMatchTotal);
}

void MainWindow::onSearchPrev()
{
    auto *t = currentTab(); if (!t || !m_searchMatchTotal) return;
    m_searchMatchIdx = (m_searchMatchIdx-1+m_searchMatchTotal) % m_searchMatchTotal;
    t->view->jumpToMatch(m_searchMatchIdx);
    m_searchBar->setMatchInfo(m_searchMatchIdx+1, m_searchMatchTotal);
}

void MainWindow::onSearchClose()
{
    m_searchBar->setVisible(false);
    m_searchOpenBtn->setVisible(true);
    if (auto *t = currentTab()) t->view->clearSearchMatches();
    m_searchBar->clearInfo();
    m_searchMatchTotal = 0;
}

// ─── Print ────────────────────────────────────────────────────────────────────
void MainWindow::onPrint()
{
    auto *t = currentTab(); if (!t) return;
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() != QDialog::Accepted) return;
    QPainter painter(&printer);
    float zoom  = printer.resolution() / 72.0f;
    int   pages = t->doc->pageCount();
    for (int i = 0; i < pages; ++i) {
        if (i > 0) printer.newPage();
        QImage img = t->doc->renderPage(i, zoom, t->view->rotation());
        if (!img.isNull()) {
            QRect r = painter.viewport();
            QSize s = img.size().scaled(r.size(), Qt::KeepAspectRatio);
            painter.setViewport(r.x(), r.y(), s.width(), s.height());
            painter.setWindow(img.rect());
            painter.drawImage(0, 0, img);
        }
    }
    setStatusMessage("Stampa completata");
}

// ─── Page ─────────────────────────────────────────────────────────────────────
void MainWindow::onCurrentPageChanged(int page)
{
    auto *t = currentTab(); if (!t) return;
    // Only update if signal is from the current tab's view
    if (qobject_cast<PdfView*>(sender()) != t->view) return;
    t->page = page - 1;
    m_pageInput->setText(QString::number(page));
    m_sbPage->setText(QString("Pag. %1 / %2").arg(page).arg(t->view->pageCount()));
    t->sidebar->setCurrentPage(page);
}

void MainWindow::onPageInputReturn()
{
    auto *t = currentTab(); if (!t) return;
    int p = m_pageInput->text().toInt();
    if (p >= 1 && p <= t->view->pageCount())
        t->view->scrollToPage(p-1);
}

void MainWindow::updatePageControls()
{
    auto *t = currentTab(); if (!t) return;
    m_pageInput->setText("1");
    m_pageTotalLbl->setText(QString("/ %1").arg(t->view->pageCount()));
    m_sbPage->setText(QString("Pag. 1 / %1").arg(t->view->pageCount()));
}

// ─── Render progress ─────────────────────────────────────────────────────────
void MainWindow::onRenderProgress(int done, int total)
{
    if (total <= 0) return;
    m_progressBar->setValue(60 + (done*40)/total);
    if (done >= total) {
        m_progressBar->setValue(100);
        QTimer::singleShot(500, this, [this]{ m_progressBar->setVisible(false); });
        if (auto *t = currentTab())
            setStatusMessage(QString("%1 pagine").arg(total));
    }
}

// ─── Copy ─────────────────────────────────────────────────────────────────────
void MainWindow::onCopyText()
{
    if (auto *t = currentTab()) t->view->copySelection();
    setStatusMessage("Testo copiato");
}

// ─── About ────────────────────────────────────────────────────────────────────
void MainWindow::onAbout()
{
    QMessageBox::about(this, "SFOGLI-ACCIO",
        "<b style='font-size:16px'>SFOGLI-ACCIO v1.0</b><br><br>"
        "Lettore PDF veloce, moderno e gratuito.<br>"
        "Motore: <b>MuPDF</b> &nbsp;|&nbsp; UI: <b>Qt6</b><br><br>"
        "Fatto con ♥ da ACCIO<br>"
        "Licenza MIT");
}

// ─── Recent files ─────────────────────────────────────────────────────────────
void MainWindow::addRecentFile(const QString &path)
{
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > MAX_RECENT) m_recentFiles.removeLast();
    rebuildRecentMenu();
}

void MainWindow::rebuildRecentMenu()
{
    if (!m_recentMenu) return;
    m_recentMenu->clear();
    m_recentFiles.erase(
        std::remove_if(m_recentFiles.begin(), m_recentFiles.end(),
                       [](const QString &p){ return !QFileInfo::exists(p); }),
        m_recentFiles.end());
    if (m_recentFiles.isEmpty()) {
        m_recentMenu->addAction("(nessun file recente)")->setEnabled(false);
        return;
    }
    for (const QString &p : m_recentFiles) {
        auto *a = m_recentMenu->addAction(QFileInfo(p).fileName());
        a->setToolTip(p);
        connect(a, &QAction::triggered, this, [this,p]{ onOpenRecent(p); });
    }
    m_recentMenu->addSeparator();
    connect(m_recentMenu->addAction("Cancella recenti"), &QAction::triggered, this, [this]{
        m_recentFiles.clear(); rebuildRecentMenu();
    });
}

void MainWindow::onOpenRecent(const QString &path)
{
    if (QFileInfo::exists(path)) openFile(path);
    else {
        QMessageBox::warning(this, "File non trovato",
            QString("Il file non esiste più:\n%1").arg(path));
        m_recentFiles.removeAll(path);
        rebuildRecentMenu();
    }
}

// ─── Zoom fit ─────────────────────────────────────────────────────────────────
float MainWindow::fitWidthZoom() const
{
    auto *t = currentTab(); if (!t) return 1.0f;
    int avail = t->view->viewport()->width() - 48;
    PageInfo pi = t->doc->pageInfo(0);
    return (pi.sizePt.width()>0 && avail>0)
        ? (float)avail/(float)pi.sizePt.width() : 1.0f;
}

float MainWindow::fitPageZoom() const
{
    auto *t = currentTab(); if (!t) return 1.0f;
    int aw = t->view->viewport()->width()-48, ah = t->view->viewport()->height()-48;
    PageInfo pi = t->doc->pageInfo(0);
    if (pi.sizePt.width()<=0||pi.sizePt.height()<=0) return 1.0f;
    return qMin((float)aw/(float)pi.sizePt.width(), (float)ah/(float)pi.sizePt.height());
}

// ─── Resize & EventFilter ────────────────────────────────────────────────────
bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == this && e->type() == QEvent::MouseMove) {
        auto *me = static_cast<QMouseEvent*>(e);
        const int border = 6;
        const QRect r = rect();
        const QPoint p = me->pos();
        bool left   = p.x() < border;
        bool right  = p.x() > r.width()  - border;
        bool top    = p.y() < border;
        bool bottom = p.y() > r.height() - border;
        if ((left && top) || (right && bottom))
            setCursor(Qt::SizeFDiagCursor);
        else if ((right && top) || (left && bottom))
            setCursor(Qt::SizeBDiagCursor);
        else if (left || right)
            setCursor(Qt::SizeHorCursor);
        else if (top || bottom)
            setCursor(Qt::SizeVerCursor);
        else
            setCursor(Qt::ArrowCursor);
    }
    return QMainWindow::eventFilter(obj, e);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        const int border = 6;
        const QRect r = rect();
        const QPoint p = e->pos();
        bool left   = p.x() < border;
        bool right  = p.x() > r.width()  - border;
        bool top    = p.y() < border;
        bool bottom = p.y() > r.height() - border;
        if (left || right || top || bottom) {
            Qt::Edges edges;
            if (left)   edges |= Qt::LeftEdge;
            if (right)  edges |= Qt::RightEdge;
            if (top)    edges |= Qt::TopEdge;
            if (bottom) edges |= Qt::BottomEdge;
            windowHandle()->startSystemResize(edges);
            return;
        }
    }
    QMainWindow::mousePressEvent(e);
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    QMainWindow::mouseMoveEvent(e);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{
    QMainWindow::mouseReleaseEvent(e);
}

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG *msg = static_cast<MSG*>(message);
    if (msg->message == WM_NCHITTEST) {
        const int border = 6;
        RECT winRect;
        GetWindowRect(msg->hwnd, &winRect);
        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);
        bool left   = x < winRect.left   + border;
        bool right  = x > winRect.right  - border;
        bool top    = y < winRect.top    + border;
        bool bottom = y > winRect.bottom - border;
        if (left  && top)    { *result = HTTOPLEFT;     return true; }
        if (right && top)    { *result = HTTOPRIGHT;    return true; }
        if (left  && bottom) { *result = HTBOTTOMLEFT;  return true; }
        if (right && bottom) { *result = HTBOTTOMRIGHT; return true; }
        if (left)            { *result = HTLEFT;        return true; }
        if (right)           { *result = HTRIGHT;       return true; }
        if (top)             { *result = HTTOP;         return true; }
        if (bottom)          { *result = HTBOTTOM;      return true; }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

// ─── Drag & Drop ─────────────────────────────────────────────────────────────
void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    for (const QUrl &url : e->mimeData()->urls()) {
        QString path = url.toLocalFile();
        if (path.endsWith(".pdf", Qt::CaseInsensitive)) openFile(path);
    }
}

// ─── Close ────────────────────────────────────────────────────────────────────
void MainWindow::closeEvent(QCloseEvent *e)
{
    QSettings s("SfogliAccio", "SfogliAccio");
    s.setValue("geometry",    saveGeometry());
    s.setValue("recentFiles", m_recentFiles);
    e->accept();
}

// ─── Helpers ─────────────────────────────────────────────────────────────────
void MainWindow::setStatusMessage(const QString &msg)
{
    if (m_sbStatus) m_sbStatus->setText(msg);
}

QString MainWindow::formatFileSize(qint64 b)
{
    if (b < 1024)    return QString("%1 B").arg(b);
    if (b < 1048576) return QString("%1 KB").arg(b/1024);
    return QString("%1 MB").arg(b/1048576.0, 0,'f',2);
}
