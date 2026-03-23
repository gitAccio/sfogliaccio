#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QSplitter>
#include <QProgressBar>
#include <QTimer>
#include <QAction>
#include <QMenu>
#include <QStringList>
#include <QTabBar>
#include "PdfDocument.h"
#include "PdfView.h"
#include "SidebarWidget.h"
#include "SearchBar.h"
#include "TitleBar.h"
#include "ZoomController.h"

// One tab = one open PDF
struct PdfTab {
    PdfDocument   *doc      = nullptr;
    PdfView       *view     = nullptr;
    SidebarWidget *sidebar  = nullptr;
    QSplitter     *splitter = nullptr;
    QString        path;
    QString        title;
    float          zoom     = 1.0f;
    int            page     = 0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openFile(const QString &path);

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void closeEvent(QCloseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *e) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private slots:
    void onOpenFile();
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onNewTab();
    void onFitWidth();
    void onFitPage();
    void onRotateCW();
    void onRotateCCW();
    void onToggleInvert();
    void onToggleSidebar();
    void onPrint();
    void onSearch();
    void onSearchRequested(const QString &q);
    void onSearchNext();
    void onSearchPrev();
    void onSearchClose();
    void onCurrentPageChanged(int page);
    void onPageInputReturn();
    void onRenderProgress(int done, int total);
    void onAbout();
    void onZoomChangeRequested(float zoom);
    void onOpenRecent(const QString &path);
    void onCopyText();

private:
    void buildUI();
    void buildTabBar();
    void buildToolbar();
    void buildShortcuts();
    void buildStatusBar();

    void     switchToTab(int index);
    void     closeTab(int index);
    PdfTab  *currentTab();
    PdfTab  *currentTab() const;
    int      tabIndexForPath(const QString &path) const;

    void updatePageControls();
    void updateToolbarForTab(PdfTab *tab);
    void setStatusMessage(const QString &msg);
    float fitWidthZoom() const;
    float fitPageZoom()  const;
    static QString formatFileSize(qint64 bytes);

    void addRecentFile(const QString &path);
    void rebuildRecentMenu();
    static constexpr int MAX_RECENT = 10;

    // ── Widgets ───────────────────────────────────────────────────────────────
    TitleBar       *m_titleBar      = nullptr;
    QTabBar        *m_tabBar        = nullptr;
    QWidget        *m_toolbar       = nullptr;
    QStackedWidget *m_tabStack      = nullptr;  // one splitter per tab
    QWidget        *m_welcome       = nullptr;
    SearchBar      *m_searchBar     = nullptr;
    ZoomController *m_zoom          = nullptr;
    QPushButton    *m_sidebarBtn    = nullptr;
    QWidget        *m_navGroup      = nullptr;
    QPushButton    *m_invertBtn     = nullptr;
    QPushButton    *m_searchOpenBtn = nullptr;
    QLineEdit      *m_pageInput     = nullptr;
    QLabel         *m_pageTotalLbl  = nullptr;
    QProgressBar   *m_progressBar   = nullptr;

    QLabel *m_sbFile   = nullptr;
    QLabel *m_sbSize   = nullptr;
    QLabel *m_sbPage   = nullptr;
    QLabel *m_sbZoom   = nullptr;
    QLabel *m_sbStatus = nullptr;

    QMenu      *m_recentMenu = nullptr;
    QStringList m_recentFiles;

    // ── Tab state ─────────────────────────────────────────────────────────────
    QList<PdfTab*> m_tabs;
    int            m_currentTabIdx = -1;
    QPushButton   *m_newTabBtn     = nullptr;

    bool m_sidebarOpen      = false;
    int  m_searchMatchIdx   = 0;
    int  m_searchMatchTotal = 0;
};
