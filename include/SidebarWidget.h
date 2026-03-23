#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QScrollBar>
#include "PdfDocument.h"

class SidebarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SidebarWidget(QWidget *parent = nullptr);
    void setDocument(PdfDocument *doc, int pageCount);
    void setCurrentPage(int page); // 1-based

    // Segnalibri personali
    void addPersonalBookmark(int page, const QString &title = {});
    void loadPersonalBookmarks(const QString &filePath);
    void savePersonalBookmarks(const QString &filePath);

signals:
    void pageRequested(int index); // 0-based

private slots:
    void onThumbnailClicked(int row);
    void onBookmarkClicked(QTreeWidgetItem *item);
    void onPersonalBookmarkClicked(QListWidgetItem *item);
    void onAddPersonalBookmark();
    void onRemovePersonalBookmark();

private:
    void buildThumbnails(PdfDocument *doc, int count);
    void buildBookmarks(PdfDocument *doc);
    void buildInfo(PdfDocument *doc, int count);
    void rebuildPersonalBookmarks();

    QTabWidget  *m_tabs;
    QListWidget *m_thumbList;
    QTreeWidget *m_bookmarkTree;
    QFormLayout *m_infoLayout;
    QListWidget *m_personalList;  // segnalibri personali
    QPushButton *m_addBmBtn;
    QPushButton *m_removeBmBtn;

    struct PersonalBM { int page; QString title; };
    QList<PersonalBM> m_personalBookmarks;

    bool         m_updatingSelection = false;
    int          m_currentPage = 1;
};
