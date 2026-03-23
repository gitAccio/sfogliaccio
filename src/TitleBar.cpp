#include "TitleBar.h"
#include "Theme.h"
#include <QHBoxLayout>
#include <QApplication>

TitleBar::TitleBar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(Theme::TITLEBAR_H);
    setStyleSheet(QString(
        "TitleBar{background:%1;border-bottom:1px solid %2;}")
        .arg(Theme::SURFACE).arg(Theme::BORDER));

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // ── App name ──────────────────────────────────────────────────────────────
    m_appName = new QLabel("SFOGLI-ACCIO", this);
    m_appName->setFixedHeight(Theme::TITLEBAR_H);
    m_appName->setContentsMargins(14, 0, 12, 0);
    m_appName->setStyleSheet(QString(
        "font-size:11px;font-weight:800;color:%1;"
        "letter-spacing:2px;background:transparent;border:none;")
        .arg(Theme::ACCENT));

    // ── Tab bar ───────────────────────────────────────────────────────────────
    m_tabBar = new QTabBar(this);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setDrawBase(false);
    m_tabBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_tabBar->setFixedHeight(Theme::TITLEBAR_H);
    m_tabBar->setStyleSheet(QString(R"(
        QTabBar {
            background: transparent;
        }
        QTabBar::tab {
            background: transparent;
            color: %1;
            padding: 0 14px;
            height: %2px;
            min-width: 100px;
            max-width: 200px;
            border-right: 1px solid %3;
            font-size: 12px;
        }
        QTabBar::tab:selected {
            background: %4;
            color: %5;
            border-bottom: 2px solid %5;
        }
        QTabBar::tab:hover:!selected {
            background: %6;
            color: %7;
        }
    )").arg(Theme::TEXT_DIM).arg(Theme::TITLEBAR_H)
       .arg(Theme::BORDER)
       .arg(Theme::SURFACE2).arg(Theme::ACCENT)
       .arg(Theme::SURFACE3).arg(Theme::TEXT));

    // ── New tab button ────────────────────────────────────────────────────────
    m_newTabBtn = new QPushButton("+", this);
    m_newTabBtn->setFixedSize(Theme::TITLEBAR_H, Theme::TITLEBAR_H);
    m_newTabBtn->setToolTip("Nuovo tab (Ctrl+T)");
    m_newTabBtn->setStyleSheet(QString(
        "QPushButton{background:transparent;color:%1;border:none;"
        "font-size:18px;font-weight:300;}"
        "QPushButton:hover{background:%2;color:%3;}")
        .arg(Theme::TEXT_DIM).arg(Theme::SURFACE3).arg(Theme::ACCENT));

    // ── Window controls ───────────────────────────────────────────────────────
    QString btnBase = QString(
        "QPushButton{background:transparent;color:%1;"
        "border:none;font-size:13px;width:46px;}"
        "QPushButton:hover{background:%2;color:%3;}")
        .arg(Theme::TEXT_DIM).arg(Theme::SURFACE3).arg(Theme::TEXT);
    QString closeCss = QString(
        "QPushButton{background:transparent;color:%1;"
        "border:none;font-size:13px;width:46px;}"
        "QPushButton:hover{background:%2;color:white;}")
        .arg(Theme::TEXT_DIM).arg(Theme::DANGER);

    m_minBtn   = new QPushButton("—", this);
    m_maxBtn   = new QPushButton("□", this);
    m_closeBtn = new QPushButton("✕", this);
    m_minBtn->setFixedSize(46, Theme::TITLEBAR_H);
    m_maxBtn->setFixedSize(46, Theme::TITLEBAR_H);
    m_closeBtn->setFixedSize(46, Theme::TITLEBAR_H);
    m_minBtn->setStyleSheet(btnBase);
    m_maxBtn->setStyleSheet(btnBase);
    m_closeBtn->setStyleSheet(closeCss);
    m_minBtn->setToolTip("Minimizza");
    m_maxBtn->setToolTip("Massimizza");
    m_closeBtn->setToolTip("Chiudi");

    // ── Layout ────────────────────────────────────────────────────────────────
    lay->addWidget(m_appName, 0);
    lay->addWidget(m_tabBar, 1);
    lay->addWidget(m_newTabBtn, 0);
    lay->addWidget(m_minBtn, 0);
    lay->addWidget(m_maxBtn, 0);
    lay->addWidget(m_closeBtn, 0);

    connect(m_minBtn,   &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maxBtn,   &QPushButton::clicked, this, &TitleBar::maximizeRequested);
    connect(m_closeBtn, &QPushButton::clicked, this, &TitleBar::closeRequested);
}

void TitleBar::setFileName(const QString &) {}  // no-op, kept for compatibility

void TitleBar::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging  = true;
        m_dragStart = e->globalPosition().toPoint() - window()->frameGeometry().topLeft();
    }
}
void TitleBar::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton))
        window()->move(e->globalPosition().toPoint() - m_dragStart);
}
void TitleBar::mouseReleaseEvent(QMouseEvent *) { m_dragging = false; }
void TitleBar::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) emit maximizeRequested();
}
