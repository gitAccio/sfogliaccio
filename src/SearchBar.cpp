#include "SearchBar.h"
#include "Theme.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>

SearchBar::SearchBar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(36);
    setStyleSheet(QString(
        "QWidget { background: %1; border-top: 1px solid %2; }")
        .arg(Theme::SURFACE).arg(Theme::BORDER));

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12,4,12,4);
    lay->setSpacing(6);

    auto *icon = new QLabel("⌕", this);
    icon->setStyleSheet(QString("color:%1; font-size:14px; background:transparent;")
        .arg(Theme::TEXT_DIMMER));

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Cerca nel PDF…");
    m_input->setStyleSheet(QString(
        "QLineEdit {"
        "  background:%1; border:1px solid %2; border-radius:3px;"
        "  color:%3; font-family:'JetBrains Mono',monospace; font-size:13px;"
        "  padding:0 8px; height:32px;"
        "}"
        "QLineEdit:focus { border-color:%4; }")
        .arg(Theme::SURFACE3).arg(Theme::BORDER2)
        .arg(Theme::TEXT).arg(Theme::ACCENT2));
    m_input->setMinimumWidth(200);
    m_input->installEventFilter(this);

    m_countLabel = new QLabel(this);
    m_countLabel->setFixedWidth(80);
    m_countLabel->setAlignment(Qt::AlignCenter);
    m_countLabel->setStyleSheet(
        QString("color:%1; font-size:12px; background:transparent;").arg(Theme::TEXT_DIM));

    QString btnStyle = QString(
        "QPushButton {"
        "  background:transparent; color:%1;"
        "  border:1px solid transparent; border-radius:3px;"
        "  padding:0 7px; height:32px; font-size:13px;"
        "}"
        "QPushButton:hover { background:%2; border-color:%3; color:%4; }")
        .arg(Theme::TEXT_DIM).arg(Theme::SURFACE3).arg(Theme::BORDER2).arg(Theme::TEXT);

    m_prevBtn  = new QPushButton("↑", this);
    m_nextBtn  = new QPushButton("↓", this);
    m_closeBtn = new QPushButton("✕", this);
    for (auto *b : {m_prevBtn, m_nextBtn, m_closeBtn}) {
        b->setFixedSize(32,32);
        b->setStyleSheet(btnStyle);
    }
    m_prevBtn->setToolTip("Precedente (Shift+Enter)");
    m_nextBtn->setToolTip("Successivo (Enter)");
    m_closeBtn->setToolTip("Chiudi (Esc)");

    lay->addWidget(icon);
    lay->addWidget(m_input, 1);
    lay->addWidget(m_countLabel);
    lay->addWidget(m_prevBtn);
    lay->addWidget(m_nextBtn);
    lay->addWidget(m_closeBtn);

    connect(m_input,    &QLineEdit::textChanged, this, &SearchBar::searchRequested);
    connect(m_prevBtn,  &QPushButton::clicked,   this, &SearchBar::navigatePrev);
    connect(m_nextBtn,  &QPushButton::clicked,   this, &SearchBar::navigateNext);
    connect(m_closeBtn, &QPushButton::clicked,   this, &SearchBar::closed);
}

QString SearchBar::text() const { return m_input->text(); }

void SearchBar::setMatchInfo(int current, int total)
{
    if (total == 0) m_countLabel->setText("0 risultati");
    else            m_countLabel->setText(QString("%1 / %2").arg(current).arg(total));
}

void SearchBar::clearInfo() { m_countLabel->clear(); }

void SearchBar::focusInput()
{
    m_input->setFocus();
    m_input->selectAll();
}

bool SearchBar::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_input && ev->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(ev);
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            if (ke->modifiers() & Qt::ShiftModifier) emit navigatePrev();
            else                                      emit navigateNext();
            return true;
        }
        if (ke->key() == Qt::Key_Escape) { emit closed(); return true; }
    }
    return QWidget::eventFilter(obj, ev);
}
