#pragma once
#include <QWidget>
#include <QPushButton>
#include <QLabel>

class ZoomController : public QWidget
{
    Q_OBJECT
public:
    explicit ZoomController(QWidget *parent = nullptr);
    float zoom() const { return m_zoom; }
    void  setZoom(float z);

signals:
    void zoomChanged(float zoom);

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

private:
    void updateLabel();
    QPushButton *m_inBtn;
    QPushButton *m_outBtn;
    QLabel      *m_label;
    float        m_zoom = 1.0f;
    static constexpr float MIN_ZOOM  = 0.1f;
    static constexpr float MAX_ZOOM  = 6.0f;
    static constexpr float ZOOM_STEP = 0.15f;
};
