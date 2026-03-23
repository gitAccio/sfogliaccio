#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QFormLayout>
#include "PdfDocument.h"

class SidebarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SidebarWidget(QWidget *parent = nullptr);
    void setDocument(PdfDocument *doc, int pageCount);
    void setCurrentPage(int page); // 1-based

signals:
    void pageRequested(int index); // 0-based

private slots:
    void onThumbnailClicked(int row);
    void onBookmarkClicked(QTreeWidgetItem *item);

private:
    void buildThumbnails(PdfDocument *doc, int count);
    void buildBookmarks(PdfDocument *doc);
    void buildInfo(PdfDocument *doc, int count);

    QTabWidget  *m_tabs;
    QListWidget *m_thumbList;
    QTreeWidget *m_bookmarkTree;
    QFormLayout *m_infoLayout;
    bool         m_updatingSelection = false;
};
