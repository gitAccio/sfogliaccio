#include "SearchBar.h"
#include "Theme.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QKeyEvent>

SearchBar::SearchBar(QWidget *parent) : QWidget(parent)
{
    setStyleSheet(QString(
        "SearchBar { background: %1; border-top: 1px solid %2; }")
        .arg(Theme::SURFACE).arg(Theme::BORDER));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // ── Barra di progresso ricerca (in cima, inizialmente nascosta) ───────────
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);   // indeterminate
    m_progressBar->setFixedHeight(3);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(QString(
        "QProgressBar { background:%1; border:none; border-radius:0; }"
        "QProgressBar::chunk { background:%2; border-radius:0; }")
        .arg(Theme::SURFACE).arg(Theme::ACCENT2));
    m_progressBar->hide();

    // ── Riga principale ───────────────────────────────────────────────────────
    auto *row = new QWidget(this);
    row->setFixedHeight(36);
    row->setStyleSheet("background:transparent;");
    auto *lay = new QHBoxLayout(row);
    lay->setContentsMargins(12,2,12,2);
    lay->setSpacing(6);

    auto *icon = new QLabel("⌕", row);
    icon->setStyleSheet(QString("color:%1; font-size:16px; background:transparent;")
        .arg(Theme::TEXT_DIMMER));

    m_input = new QLineEdit(row);
    m_input->setPlaceholderText("Cerca nel PDF…");
    m_input->setStyleSheet(QString(R"(
        QLineEdit {
            background:%1; border:1px solid %2; border-radius:4px;
            color:%3; font-size:13px; padding:0 8px; height:28px;
        }
        QLineEdit:focus { border-color:%4; }
    )").arg(Theme::SURFACE3).arg(Theme::BORDER2)
       .arg(Theme::TEXT).arg(Theme::ACCENT2));
    m_input->setMinimumWidth(220);
    m_input->installEventFilter(this);

    // Spinner "in ricerca..."
    m_spinnerLabel = new QLabel("  ⟳ ricerca...", row);
    m_spinnerLabel->setStyleSheet(QString(
        "color:%1; font-size:11px; background:transparent;")
        .arg(Theme::ACCENT2));
    m_spinnerLabel->hide();

    m_countLabel = new QLabel(row);
    m_countLabel->setMinimumWidth(80);
    m_countLabel->setAlignment(Qt::AlignCenter);
    m_countLabel->setStyleSheet(
        QString("color:%1; font-size:12px; background:transparent;")
        .arg(Theme::TEXT_DIM));

    QString btnStyle = QString(R"(
        QPushButton {
            background:transparent; color:%1;
            border:1px solid transparent; border-radius:3px;
            padding:0 7px; height:28px; font-size:14px;
        }
        QPushButton:hover { background:%2; border-color:%3; color:%4; }
    )").arg(Theme::TEXT_DIM).arg(Theme::SURFACE3)
       .arg(Theme::BORDER2).arg(Theme::TEXT);

    m_prevBtn  = new QPushButton("↑", row);
    m_nextBtn  = new QPushButton("↓", row);
    m_closeBtn = new QPushButton("✕", row);
    for (auto *b : {m_prevBtn, m_nextBtn, m_closeBtn}) {
        b->setFixedSize(30, 28);
        b->setStyleSheet(btnStyle);
    }
    m_prevBtn->setToolTip("Precedente (Shift+Enter)");
    m_nextBtn->setToolTip("Successivo (Enter)");
    m_closeBtn->setToolTip("Chiudi (Esc)");

    lay->addWidget(icon);
    lay->addWidget(m_input, 1);
    lay->addWidget(m_spinnerLabel);
    lay->addWidget(m_countLabel);
    lay->addWidget(m_prevBtn);
    lay->addWidget(m_nextBtn);
    lay->addWidget(m_closeBtn);

    root->addWidget(m_progressBar);
    root->addWidget(row);

    connect(m_input,    &QLineEdit::textChanged, this, &SearchBar::searchRequested);
    connect(m_prevBtn,  &QPushButton::clicked,   this, &SearchBar::navigatePrev);
    connect(m_nextBtn,  &QPushButton::clicked,   this, &SearchBar::navigateNext);
    connect(m_closeBtn, &QPushButton::clicked,   this, &SearchBar::closed);
}

QString SearchBar::text() const { return m_input->text(); }

void SearchBar::setMatchInfo(int current, int total)
{
    if (total == 0)
        m_countLabel->setText("0 risultati");
    else
        m_countLabel->setText(QString("%1 / %2").arg(current).arg(total));
}

void SearchBar::clearInfo()
{
    m_countLabel->clear();
    setSearching(false);
}

void SearchBar::setSearching(bool on)
{
    m_spinnerLabel->setVisible(on);
    m_progressBar->setVisible(on);
    if (!on) m_progressBar->hide();
}

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
