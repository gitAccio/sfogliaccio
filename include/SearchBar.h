#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>

class SearchBar : public QWidget
{
    Q_OBJECT
public:
    explicit SearchBar(QWidget *parent = nullptr);
    QString text() const;
    void setMatchInfo(int current, int total);
    void clearInfo();
    void focusInput();
    void setSearching(bool on);   // mostra/nasconde spinner ricerca

signals:
    void searchRequested(const QString &query);
    void navigateNext();
    void navigatePrev();
    void closed();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    QLineEdit    *m_input;
    QLabel       *m_countLabel;
    QLabel       *m_spinnerLabel;
    QPushButton  *m_prevBtn;
    QPushButton  *m_nextBtn;
    QPushButton  *m_closeBtn;
    QProgressBar *m_progressBar;
};
