#include "TitleBar.h"
#include "Theme.h"
#include <QHBoxLayout>

TitleBar::TitleBar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(Theme::TITLEBAR_H);
    setStyleSheet(QString("QWidget{background:%1;border-bottom:1px solid %2;}")
        .arg(Theme::SURFACE).arg(Theme::BORDER));

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 0, 0, 0);
    lay->setSpacing(8);

    m_appName = new QLabel("SFOGLI-ACCIO", this);
    m_appName->setStyleSheet(QString(
        "font-size:11px;font-weight:800;color:%1;"
        "letter-spacing:2px;background:transparent;border:none;")
        .arg(Theme::ACCENT));

    m_fileName = new QLabel("— Nessun file", this);
    m_fileName->setStyleSheet(QString(
        "font-size:11px;color:%1;background:transparent;border:none;")
        .arg(Theme::TEXT_DIM));
    m_fileName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    lay->addWidget(m_appName);
    lay->addWidget(m_fileName);
    lay->addStretch();

    QString btnBase = QString(
        "QPushButton{background:transparent;color:%1;"
        "border:none;font-size:11px;}"
        "QPushButton:hover{background:%2;color:%3;}")
        .arg(Theme::TEXT_DIM).arg(Theme::SURFACE3).arg(Theme::TEXT);
    QString closeCss = QString(
        "QPushButton{background:transparent;color:%1;border:none;font-size:11px;}"
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

    lay->addWidget(m_minBtn);
    lay->addWidget(m_maxBtn);
    lay->addWidget(m_closeBtn);

    connect(m_minBtn,   &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maxBtn,   &QPushButton::clicked, this, &TitleBar::maximizeRequested);
    connect(m_closeBtn, &QPushButton::clicked, this, &TitleBar::closeRequested);
}

void TitleBar::setFileName(const QString &name)
{
    m_fileName->setText(name.isEmpty() ? "— Nessun file" : "— " + name);
}

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
