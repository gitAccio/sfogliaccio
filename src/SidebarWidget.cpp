#include "SidebarWidget.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <QInputDialog>
#include <QSettings>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QListWidgetItem>

SidebarWidget::SidebarWidget(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(Theme::SIDEBAR_W);
    setStyleSheet(QString(
        "QWidget { background:%1; border-right:1px solid %2; }")
        .arg(Theme::SURFACE).arg(Theme::BORDER));

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setStyleSheet(QString(R"(
        QTabWidget::pane { border:none; background:%1; }
        QTabBar::tab {
            background:transparent; color:%2;
            padding:6px 0; font-size:9px;
            letter-spacing:1.5px; text-transform:uppercase;
            border-bottom:2px solid transparent; min-width:55px;
        }
        QTabBar::tab:selected  { color:%3; border-bottom-color:%3; }
        QTabBar::tab:hover:!selected { color:%4; }
    )").arg(Theme::SURFACE).arg(Theme::TEXT_DIMMER)
       .arg(Theme::ACCENT).arg(Theme::TEXT_DIM));

    QString listStyle = QString(R"(
        QListWidget { background:%1; border:none; }
        QListWidget::item {
            background:transparent; border:1px solid transparent;
            border-radius:3px; padding:4px; color:%2;
        }
        QListWidget::item:hover    { background:%3; border-color:%4; }
        QListWidget::item:selected { background:%5; border-color:%6; }
    )").arg(Theme::SURFACE).arg(Theme::TEXT_DIM)
       .arg(Theme::SURFACE2).arg(Theme::BORDER)
       .arg(Theme::ACCENT_DIM).arg(Theme::ACCENT);

    QString treeStyle = QString(R"(
        QTreeWidget { background:%1; border:none; color:%2; font-size:12px; }
        QTreeWidget::item { padding:4px 8px; border:none; }
        QTreeWidget::item:hover    { background:%3; color:%4; }
        QTreeWidget::item:selected { background:%5; color:%4; border-left:2px solid %6; }
        QTreeWidget::branch { background:%1; }
    )").arg(Theme::SURFACE).arg(Theme::TEXT_DIM)
       .arg(Theme::SURFACE2).arg(Theme::TEXT)
       .arg(Theme::SURFACE3).arg(Theme::ACCENT2);

    // ── Tab Pagine ────────────────────────────────────────────────────────────
    m_thumbList = new QListWidget(this);
    m_thumbList->setStyleSheet(listStyle);
    m_thumbList->setIconSize(QSize(160,220));
    m_thumbList->setSpacing(4);
    m_thumbList->setResizeMode(QListView::Adjust);
    connect(m_thumbList, &QListWidget::currentRowChanged,
            this, &SidebarWidget::onThumbnailClicked);

    // ── Tab Segnalibri PDF ────────────────────────────────────────────────────
    m_bookmarkTree = new QTreeWidget(this);
    m_bookmarkTree->setHeaderHidden(true);
    m_bookmarkTree->setStyleSheet(treeStyle);
    m_bookmarkTree->setIndentation(16);
    connect(m_bookmarkTree, &QTreeWidget::itemClicked,
            this, &SidebarWidget::onBookmarkClicked);

    // ── Tab Preferiti (segnalibri personali) ──────────────────────────────────
    auto *favWidget = new QWidget(this);
    favWidget->setStyleSheet(QString("background:%1;").arg(Theme::SURFACE));
    auto *favLay = new QVBoxLayout(favWidget);
    favLay->setContentsMargins(0,0,0,0);
    favLay->setSpacing(0);

    // Pulsanti aggiungi/rimuovi
    auto *btnRow = new QWidget(favWidget);
    btnRow->setStyleSheet(QString(
        "background:%1; border-bottom:1px solid %2;")
        .arg(Theme::SURFACE).arg(Theme::BORDER));
    auto *btnL = new QHBoxLayout(btnRow);
    btnL->setContentsMargins(6,4,6,4);
    btnL->setSpacing(4);

    m_addBmBtn    = new QPushButton("+ Aggiungi", btnRow);
    m_removeBmBtn = new QPushButton("✕ Rimuovi",  btnRow);
    QString bmBtnStyle = QString(
        "QPushButton{background:transparent;color:%1;border:1px solid %2;"
        "border-radius:3px;font-size:11px;padding:3px 8px;}"
        "QPushButton:hover{background:%3;color:%4;border-color:%4;}")
        .arg(Theme::TEXT_DIM).arg(Theme::BORDER)
        .arg(Theme::SURFACE2).arg(Theme::ACCENT);
    m_addBmBtn->setStyleSheet(bmBtnStyle);
    m_removeBmBtn->setStyleSheet(bmBtnStyle);
    btnL->addWidget(m_addBmBtn);
    btnL->addWidget(m_removeBmBtn);
    btnL->addStretch();

    m_personalList = new QListWidget(favWidget);
    m_personalList->setStyleSheet(listStyle);
    connect(m_personalList, &QListWidget::itemClicked,
            this, &SidebarWidget::onPersonalBookmarkClicked);
    connect(m_addBmBtn,    &QPushButton::clicked, this, &SidebarWidget::onAddPersonalBookmark);
    connect(m_removeBmBtn, &QPushButton::clicked, this, &SidebarWidget::onRemovePersonalBookmark);

    favLay->addWidget(btnRow);
    favLay->addWidget(m_personalList, 1);

    // ── Tab Info ──────────────────────────────────────────────────────────────
    auto *infoScroll = new QScrollArea(this);
    infoScroll->setFrameShape(QFrame::NoFrame);
    infoScroll->setStyleSheet(QString("background:%1;").arg(Theme::SURFACE));
    auto *infoInner = new QWidget(infoScroll);
    infoInner->setStyleSheet(QString("background:%1;").arg(Theme::SURFACE));
    m_infoLayout = new QFormLayout(infoInner);
    m_infoLayout->setContentsMargins(10,10,10,10);
    m_infoLayout->setSpacing(0);
    m_infoLayout->setLabelAlignment(Qt::AlignLeft);
    infoScroll->setWidget(infoInner);
    infoScroll->setWidgetResizable(true);

    m_tabs->addTab(m_thumbList,    "Pagine");
    m_tabs->addTab(m_bookmarkTree, "Indice");
    m_tabs->addTab(favWidget,      "★ Preferiti");
    m_tabs->addTab(infoScroll,     "Info");

    lay->addWidget(m_tabs);
}

void SidebarWidget::setDocument(PdfDocument *doc, int pageCount)
{
    m_thumbList->clear();
    m_bookmarkTree->clear();
    while (m_infoLayout->rowCount()) m_infoLayout->removeRow(0);
    m_personalBookmarks.clear();
    m_personalList->clear();

    if (!doc) return;

    buildThumbnails(doc, pageCount);
    buildBookmarks(doc);
    buildInfo(doc, pageCount);
}

void SidebarWidget::setCurrentPage(int page)
{
    m_currentPage = page;
    if (m_thumbList->count() == 0) return;
    int idx = page - 1;
    if (idx < 0 || idx >= m_thumbList->count()) return;
    m_updatingSelection = true;
    m_thumbList->setCurrentRow(idx);
    m_thumbList->scrollToItem(m_thumbList->item(idx), QAbstractItemView::EnsureVisible);
    m_updatingSelection = false;
}

// ── Segnalibri personali ──────────────────────────────────────────────────────
void SidebarWidget::addPersonalBookmark(int page, const QString &title)
{
    // Evita duplicati sulla stessa pagina
    for (const auto &bm : m_personalBookmarks)
        if (bm.page == page) return;

    PersonalBM bm;
    bm.page  = page;
    bm.title = title.isEmpty() ? QString("Pagina %1").arg(page) : title;
    m_personalBookmarks.append(bm);
    rebuildPersonalBookmarks();
}

void SidebarWidget::loadPersonalBookmarks(const QString &filePath)
{
    m_personalBookmarks.clear();
    QSettings s("SfogliAccio", "SfogliAccio");
    QString key = QString("bookmarks/%1").arg(QString(filePath.toUtf8().toHex()));
    int n = s.beginReadArray(key);
    for (int i = 0; i < n; ++i) {
        s.setArrayIndex(i);
        PersonalBM bm;
        bm.page  = s.value("page").toInt();
        bm.title = s.value("title").toString();
        m_personalBookmarks.append(bm);
    }
    s.endArray();
    rebuildPersonalBookmarks();
}

void SidebarWidget::savePersonalBookmarks(const QString &filePath)
{
    QSettings s("SfogliAccio", "SfogliAccio");
    QString key = QString("bookmarks/%1").arg(QString(filePath.toUtf8().toHex()));
    s.beginWriteArray(key);
    for (int i = 0; i < m_personalBookmarks.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue("page",  m_personalBookmarks[i].page);
        s.setValue("title", m_personalBookmarks[i].title);
    }
    s.endArray();
}

void SidebarWidget::rebuildPersonalBookmarks()
{
    m_personalList->clear();
    // Ordina per pagina
    auto sorted = m_personalBookmarks;
    std::sort(sorted.begin(), sorted.end(),
              [](const PersonalBM &a, const PersonalBM &b){ return a.page < b.page; });
    for (const auto &bm : sorted) {
        auto *item = new QListWidgetItem(
            QString("⭐ %1  —  pag. %2").arg(bm.title).arg(bm.page));
        item->setData(Qt::UserRole, bm.page - 1); // 0-based
        item->setToolTip(QString("Vai a pagina %1").arg(bm.page));
        m_personalList->addItem(item);
    }
}

void SidebarWidget::onAddPersonalBookmark()
{
    bool ok;
    QString title = QInputDialog::getText(this, "Aggiungi preferito",
        QString("Nome per pagina %1:").arg(m_currentPage),
        QLineEdit::Normal,
        QString("Pagina %1").arg(m_currentPage), &ok);
    if (!ok) return;
    addPersonalBookmark(m_currentPage, title);
}

void SidebarWidget::onRemovePersonalBookmark()
{
    auto *item = m_personalList->currentItem();
    if (!item) return;
    int page0 = item->data(Qt::UserRole).toInt(); // 0-based
    m_personalBookmarks.removeIf([page0](const PersonalBM &bm){
        return bm.page == page0 + 1;
    });
    rebuildPersonalBookmarks();
}

void SidebarWidget::onPersonalBookmarkClicked(QListWidgetItem *item)
{
    if (!item) return;
    emit pageRequested(item->data(Qt::UserRole).toInt());
}

// ── Build helpers ─────────────────────────────────────────────────────────────
void SidebarWidget::buildThumbnails(PdfDocument *doc, int count)
{
    const int THUMB_W = 160;
    // Per PDF grandi, limita le thumbnail iniziali e carica le altre lazy
    for (int i = 0; i < count; ++i) {
        auto *item = new QListWidgetItem(m_thumbList);
        item->setText(QString("Pag. %1").arg(i+1));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        item->setSizeHint(QSize(180,240));

        // Carica solo le prime 20 thumbnail subito, le altre on-demand
        if (i < 20) {
            auto *watcher = new QFutureWatcher<QImage>(this);
            int idx = i;
            connect(watcher, &QFutureWatcher<QImage>::finished, this,
                    [this, idx, watcher]() {
                QImage img = watcher->result();
                if (!img.isNull() && idx < m_thumbList->count())
                    m_thumbList->item(idx)->setIcon(QIcon(QPixmap::fromImage(img)));
                watcher->deleteLater();
            });
            watcher->setFuture(QtConcurrent::run([doc, idx, THUMB_W]() {
                return doc->renderThumbnail(idx, THUMB_W);
            }));
        }
    }

    // Carica thumbnail quando diventano visibili
    connect(m_thumbList->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this, doc, count, THUMB_W]() {
        int first = m_thumbList->currentRow();
        QRect vp  = m_thumbList->viewport()->rect();
        for (int i = 0; i < count; ++i) {
            auto *item = m_thumbList->item(i);
            if (!item) continue;
            if (item->icon().isNull() &&
                m_thumbList->visualItemRect(item).intersects(vp)) {
                auto *watcher = new QFutureWatcher<QImage>(this);
                connect(watcher, &QFutureWatcher<QImage>::finished, this,
                        [this, i, watcher]() {
                    QImage img = watcher->result();
                    if (!img.isNull() && i < m_thumbList->count())
                        m_thumbList->item(i)->setIcon(QIcon(QPixmap::fromImage(img)));
                    watcher->deleteLater();
                });
                watcher->setFuture(QtConcurrent::run([doc, i, THUMB_W]() {
                    return doc->renderThumbnail(i, THUMB_W);
                }));
            }
        }
        Q_UNUSED(first);
    });
}

void SidebarWidget::buildBookmarks(PdfDocument *doc)
{
    auto bms = doc->bookmarks();
    if (bms.isEmpty()) {
        auto *item = new QTreeWidgetItem(m_bookmarkTree);
        item->setText(0, "Nessun indice disponibile");
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(0, QColor(Theme::TEXT_DIMMER));
        return;
    }

    QList<QTreeWidgetItem*> stack;
    for (const BookmarkItem &bm : bms) {
        auto *item = new QTreeWidgetItem();
        item->setText(0, bm.title);
        item->setData(0, Qt::UserRole, bm.pageIndex);
        item->setToolTip(0, QString("Pagina %1").arg(bm.pageIndex+1));

        while (stack.size() > bm.level) stack.pop_back();
        if (stack.isEmpty()) m_bookmarkTree->addTopLevelItem(item);
        else stack.last()->addChild(item);
        stack.append(item);
    }
    m_bookmarkTree->expandAll();
}

void SidebarWidget::buildInfo(PdfDocument *doc, int count)
{
    auto addRow = [this](const QString &key, const QString &val) {
        if (val.isEmpty()) return;
        auto *k = new QLabel(key);
        k->setStyleSheet(QString(
            "color:%1; font-size:9px; text-transform:uppercase;"
            "letter-spacing:1px; border-bottom:1px solid %2; padding:5px 0;")
            .arg(Theme::TEXT_DIMMER).arg(Theme::BORDER));
        auto *v = new QLabel(val);
        v->setWordWrap(true);
        v->setStyleSheet(QString(
            "color:%1; font-size:12px; border-bottom:1px solid %2; padding:5px 0;")
            .arg(Theme::TEXT).arg(Theme::BORDER));
        m_infoLayout->addRow(k, v);
    };

    addRow("Titolo",     doc->title());
    addRow("Autore",     doc->author());
    addRow("Soggetto",   doc->subject());
    addRow("Produttore", doc->producer());
    addRow("Creatore",   doc->creator());
    addRow("Pagine",     QString::number(count));
    addRow("File",       doc->filePath().section('/', -1));
}

void SidebarWidget::onThumbnailClicked(int row)
{
    if (m_updatingSelection || row < 0) return;
    emit pageRequested(row);
}

void SidebarWidget::onBookmarkClicked(QTreeWidgetItem *item)
{
    if (!item) return;
    emit pageRequested(item->data(0, Qt::UserRole).toInt());
}
