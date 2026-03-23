#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

class SearchBar : public QWidget
{
    Q_OBJECT
public:
    explicit SearchBar(QWidget *parent = nullptr);
    QString text() const;
    void setMatchInfo(int current, int total);
    void clearInfo();
    void focusInput();

signals:
    void searchRequested(const QString &query);
    void navigateNext();
    void navigatePrev();
    void closed();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    QLineEdit   *m_input;
    QLabel      *m_countLabel;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_closeBtn;
};
