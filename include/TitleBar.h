#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTabBar>
#include <QMouseEvent>
#include <QHBoxLayout>

// TitleBar integrata con tab:
// [SFOGLI-ACCIO] [tab1 ×] [tab2 ×] [+]  ···drag area···  [━][□][✕]
class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    // Tab management — delegated to internal QTabBar
    QTabBar *tabBar() { return m_tabBar; }
    QPushButton *newTabBtn() { return m_newTabBtn; }

    void setFileName(const QString &name);  // kept for compat, updates nothing visible now

signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

private:
    QLabel      *m_appName   = nullptr;
    QTabBar     *m_tabBar    = nullptr;
    QPushButton *m_newTabBtn = nullptr;
    QPushButton *m_minBtn    = nullptr;
    QPushButton *m_maxBtn    = nullptr;
    QPushButton *m_closeBtn  = nullptr;

    bool   m_dragging  = false;
    QPoint m_dragStart;
};
