#include "ZoomController.h"
#include "Theme.h"
#include <QHBoxLayout>

ZoomController::ZoomController(QWidget *parent) : QWidget(parent)
{
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(2);

    QString btnCss = QString(
        "QPushButton{background:transparent;color:%1;"
        "border:1px solid transparent;border-radius:3px;"
        "font-size:14px;padding:0;width:26px;height:32px;}"
        "QPushButton:hover{background:%2;border-color:%3;color:%4;}")
        .arg(Theme::TEXT_DIM).arg(Theme::SURFACE3).arg(Theme::BORDER2).arg(Theme::TEXT);

    m_outBtn = new QPushButton("−", this);
    m_outBtn->setFixedSize(32,32);
    m_outBtn->setStyleSheet(btnCss);
    m_outBtn->setToolTip("Zoom out (Ctrl+-)");

    m_label = new QLabel("100%", this);
    m_label->setFixedSize(50, 26);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet(QString(
        "background:%1;border:1px solid %2;border-radius:3px;"
        "color:%3;font-size:13px;")
        .arg(Theme::SURFACE3).arg(Theme::BORDER).arg(Theme::TEXT));
    m_label->setToolTip("Zoom attuale — click per 100%");
    m_label->setCursor(Qt::PointingHandCursor);

    m_inBtn = new QPushButton("+", this);
    m_inBtn->setFixedSize(32,32);
    m_inBtn->setStyleSheet(btnCss);
    m_inBtn->setToolTip("Zoom in (Ctrl+=)");

    lay->addWidget(m_outBtn);
    lay->addWidget(m_label);
    lay->addWidget(m_inBtn);

    connect(m_outBtn, &QPushButton::clicked, this, &ZoomController::zoomOut);
    connect(m_inBtn,  &QPushButton::clicked, this, &ZoomController::zoomIn);
}

void ZoomController::setZoom(float z)
{
    m_zoom = qBound(MIN_ZOOM, z, MAX_ZOOM);
    updateLabel();
    emit zoomChanged(m_zoom);
}
void ZoomController::zoomIn()    { setZoom(m_zoom + ZOOM_STEP); }
void ZoomController::zoomOut()   { setZoom(m_zoom - ZOOM_STEP); }
void ZoomController::resetZoom() { setZoom(1.0f); }
void ZoomController::updateLabel()
{
    m_label->setText(QString("%1%").arg(qRound(m_zoom * 100)));
}
