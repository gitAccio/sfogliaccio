#include "SidebarWidget.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
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
            border-bottom:2px solid transparent; min-width:60px;
        }
        QTabBar::tab:selected  { color:%3; border-bottom-color:%3; }
        QTabBar::tab:hover:!selected { color:%4; }
    )").arg(Theme::SURFACE).arg(Theme::TEXT_DIMMER)
       .arg(Theme::ACCENT).arg(Theme::TEXT_DIM));

    // ── Thumbnails ────────────────────────────────────────────────────────────
    m_thumbList = new QListWidget(this);
    m_thumbList->setStyleSheet(QString(R"(
        QListWidget { background:%1; border:none; }
        QListWidget::item {
            background:transparent; border:1px solid transparent;
            border-radius:3px; padding:4px; color:%2;
        }
        QListWidget::item:hover    { background:%3; border-color:%4; }
        QListWidget::item:selected { background:%5; border-color:%6; }
    )").arg(Theme::SURFACE).arg(Theme::TEXT_DIM)
       .arg(Theme::SURFACE2).arg(Theme::BORDER)
       .arg(Theme::ACCENT_DIM).arg(Theme::ACCENT));
    m_thumbList->setIconSize(QSize(160,220));
    m_thumbList->setSpacing(4);
    m_thumbList->setResizeMode(QListView::Adjust);

    connect(m_thumbList, &QListWidget::currentRowChanged,
            this, &SidebarWidget::onThumbnailClicked);

    // ── Bookmarks ─────────────────────────────────────────────────────────────
    m_bookmarkTree = new QTreeWidget(this);
    m_bookmarkTree->setHeaderHidden(true);
    m_bookmarkTree->setStyleSheet(QString(R"(
        QTreeWidget { background:%1; border:none; color:%2; font-size:12px; }
        QTreeWidget::item { padding:4px 8px; border:none; }
        QTreeWidget::item:hover    { background:%3; color:%4; }
        QTreeWidget::item:selected { background:%5; color:%4; border-left:2px solid %6; }
        QTreeWidget::branch { background:%1; }
    )").arg(Theme::SURFACE).arg(Theme::TEXT_DIM)
       .arg(Theme::SURFACE2).arg(Theme::TEXT)
       .arg(Theme::SURFACE3).arg(Theme::ACCENT2));
    m_bookmarkTree->setIndentation(16);

    connect(m_bookmarkTree, &QTreeWidget::itemClicked,
            this, &SidebarWidget::onBookmarkClicked);

    // ── Info ──────────────────────────────────────────────────────────────────
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
    m_tabs->addTab(m_bookmarkTree, "Segnalibri");
    m_tabs->addTab(infoScroll,     "Info");

    lay->addWidget(m_tabs);
}

void SidebarWidget::setDocument(PdfDocument *doc, int pageCount)
{
    m_thumbList->clear();
    m_bookmarkTree->clear();
    while (m_infoLayout->rowCount()) m_infoLayout->removeRow(0);

    if (!doc) return;

    buildThumbnails(doc, pageCount);
    buildBookmarks(doc);
    buildInfo(doc, pageCount);
}

void SidebarWidget::setCurrentPage(int page)
{
    if (m_thumbList->count() == 0) return;
    int idx = page - 1;
    if (idx < 0 || idx >= m_thumbList->count()) return;
    m_updatingSelection = true;
    m_thumbList->setCurrentRow(idx);
    m_thumbList->scrollToItem(m_thumbList->item(idx), QAbstractItemView::EnsureVisible);
    m_updatingSelection = false;
}

void SidebarWidget::buildThumbnails(PdfDocument *doc, int count)
{
    const int THUMB_W = 160;
    for (int i = 0; i < count; ++i) {
        auto *item = new QListWidgetItem(m_thumbList);
        item->setText(QString("Pag. %1").arg(i+1));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        item->setSizeHint(QSize(180,240));

        auto *watcher = new QFutureWatcher<QImage>(this);
        int idx = i;
        connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, idx, watcher]() {
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

void SidebarWidget::buildBookmarks(PdfDocument *doc)
{
    auto bms = doc->bookmarks();
    if (bms.isEmpty()) {
        auto *item = new QTreeWidgetItem(m_bookmarkTree);
        item->setText(0, "Nessun segnalibro");
        item->setFlags(Qt::NoItemFlags);
        return;
    }

    QList<QTreeWidgetItem*> stack;
    for (const BookmarkItem &bm : bms) {
        auto *item = new QTreeWidgetItem();
        item->setText(0, bm.title);
        item->setData(0, Qt::UserRole, bm.pageIndex);
        item->setToolTip(0, QString("Pagina %1").arg(bm.pageIndex+1));

        if (bm.level == 0 || stack.isEmpty()) {
            while (stack.size() > bm.level) stack.pop_back();
            m_bookmarkTree->addTopLevelItem(item);
        } else {
            while (stack.size() > bm.level) stack.pop_back();
            if (!stack.isEmpty()) stack.last()->addChild(item);
            else m_bookmarkTree->addTopLevelItem(item);
        }
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
