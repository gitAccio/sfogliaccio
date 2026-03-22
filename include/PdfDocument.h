#pragma once
#include <QObject>
#include <QImage>
#include <QSizeF>
#include <QString>
#include <QList>
#include <QRectF>
#include <QMutex>

extern "C" {
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
}

struct BookmarkItem {
    QString title;
    int     pageIndex = 0;
    int     level     = 0;
};

struct SearchMatch {
    int           pageIndex = 0;
    QList<QRectF> rects;
};

struct PageInfo {
    QSizeF sizePt;
};

struct TextChar {
    QRectF  bbox;
    QString ch;
};
using PageText = QList<TextChar>;

class PdfDocument : public QObject
{
    Q_OBJECT
public:
    explicit PdfDocument(QObject *parent = nullptr);
    ~PdfDocument();

    bool load(const QString &path);
    void close();
    bool isLoaded()   const { return m_loaded; }
    int  pageCount()  const { return m_pageCount; }

    PageInfo pageInfo(int index) const;
    QImage   renderPage(int index, float zoom, int rotation = 0) const;
    QImage   renderThumbnail(int index, int widthPx) const;
    PageText extractText(int pageIndex) const;

    QList<SearchMatch>  search(const QString &query) const;
    QList<BookmarkItem> bookmarks() const;

    QString metaString(const char *key) const;
    QString title()    const { return metaString("info:Title"); }
    QString author()   const { return metaString("info:Author"); }
    QString subject()  const { return metaString("info:Subject"); }
    QString creator()  const { return metaString("info:Creator"); }
    QString producer() const { return metaString("info:Producer"); }
    QString filePath() const { return m_path; }

signals:
    void loadError(const QString &msg);

private:
    void buildBookmarks(fz_outline *node, QList<BookmarkItem> &out, int level) const;

    fz_context  *m_ctx      = nullptr;
    fz_document *m_doc      = nullptr;
    QString      m_path;
    int          m_pageCount = 0;
    bool         m_loaded    = false;

    // Single mutex: all MuPDF operations are serialised.
    // Rendering is I/O + CPU bound but MuPDF is not re-entrant on a single ctx.
    mutable QMutex m_mutex;
};
