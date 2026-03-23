#pragma once
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QList>
#include <QRectF>
#include <QPixmap>
#include <QColor>
#include <QTimer>
#include "PdfDocument.h"

class PdfPageItem;
class PdfView;

// ── PdfScene ──────────────────────────────────────────────────────────────────
// Manages all page items. Pages are laid out vertically with a gap between them.
class PdfScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit PdfScene(QObject *parent = nullptr);

    void setDocument(PdfDocument *doc, float zoom, int rotation,
                     bool inverted, bool doublePage);
    void clear();

    PdfPageItem *pageItem(int index) const;
    int          pageCount() const { return m_items.size(); }
    QRectF       pageRect(int index) const;

    void setZoom(float zoom);
    void setRotation(int deg);
    void setInverted(bool inv);
    void setSearchMatches(const QList<SearchMatch> &matches);
    void clearSearchMatches();
    void setCurrentHighlight(int globalIdx);
    void setSelectionColor(const QColor &c);

    int  totalMatchCount() const { return m_totalMatches; }

    QString selectedText() const;
    void    selectAll(int pageIndex);

    static constexpr int PAGE_GAP = 16;

signals:
    void renderProgress(int done, int total);

private:
    QList<PdfPageItem*>  m_items;
    PdfDocument         *m_doc         = nullptr;
    float                m_zoom        = 1.0f;
    int                  m_rotation    = 0;
    bool                 m_inverted    = false;
    bool                 m_doublePage  = false;
    int                  m_totalMatches = 0;
    QColor               m_selColor    = QColor(77, 168, 255, 90);
};

// ── PdfPageItem ───────────────────────────────────────────────────────────────
// One page in the scene. Renders itself lazily when visible.
class PdfPageItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    explicit PdfPageItem(PdfDocument *doc, int pageIndex,
                         float zoom, int rotation, bool inverted,
                         QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void   paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                 QWidget *) override;

    // Lazy rendering — called by PdfView when this item enters viewport
    void requestRender(float zoom, int rotation, bool inverted);
    bool isRendered() const { return !m_pixmap.isNull(); }

    // Text selection
    void     setPageText(const PageText &chars, float zoom);
    void     setSelectionColor(const QColor &c) { m_selColor = c; update(); }
    QString  selectedText() const;
    void     selectAll();
    void     clearSelection();
    bool     hasSelection() const { return !m_selIdx.isEmpty(); }

    // Search highlights
    void setHighlights(const QList<QRectF> &rects, float zoom);
    void setCurrentHighlight(int idx);
    void clearHighlights();

    int pageIndex() const { return m_pageIndex; }

signals:
    void rendered(int pageIndex);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *e) override;

private:
    int charAt(const QPointF &pt) const;
    void updateSelection(int from, int to);

    PdfDocument *m_doc;
    int          m_pageIndex;
    QPixmap      m_pixmap;
    QSizeF       m_size;      // size in scene coords (pts * zoom)

    // Text
    PageText   m_chars;
    QList<int> m_selIdx;
    int        m_anchorIdx  = -1;
    bool       m_pressing   = false;
    QColor     m_selColor   = QColor(77, 168, 255, 90);

    // Highlights
    QList<QRectF> m_highlights;
    int           m_currentHL = -1;

    float m_renderedZoom     = 0;
    int   m_renderedRotation = 0;
    bool  m_renderedInverted = false;
    bool  m_renderPending    = false;
};

// ── PdfView ───────────────────────────────────────────────────────────────────
class PdfView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit PdfView(QWidget *parent = nullptr);
    ~PdfView();

    void setDocument(PdfDocument *doc);
    void rerender();

    void  setZoom(float z);
    float zoom()     const { return m_zoom; }
    void  setRotation(int deg);
    int   rotation() const { return m_rotation; }
    void  setInverted(bool inv);
    bool  inverted() const { return m_inverted; }
    void  setDoublePageMode(bool on);

    void scrollToPage(int index);
    int  currentPage() const { return m_currentPage; }
    int  pageCount()   const;

    void setSearchMatches(const QList<SearchMatch> &matches);
    void clearSearchMatches();
    void jumpToMatch(int globalIdx);
    int  totalMatchCount() const;

    QString selectedText() const;
    void    copySelection() const;
    void    selectAll();
    void    setSelectionColor(const QColor &c);

signals:
    void currentPageChanged(int page);
    void renderProgress(int done, int total);
    void zoomChangeRequested(float newZoom);
    void hasSelectionChanged(bool has);

protected:
    void wheelEvent(QWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    bool event(QEvent *e) override;
    void scrollContentsBy(int dx, int dy) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void onVisibilityCheck();

private:
    void applyZoomTransform();
    void checkVisiblePages();
    void renderPageAsync(PdfPageItem *item);

    PdfScene *m_scene       = nullptr;
    PdfDocument *m_doc      = nullptr;

    float m_zoom            = 1.0f;
    int   m_rotation        = 0;
    bool  m_inverted        = false;
    bool  m_doublePage      = false;
    int   m_currentPage     = 0;

    // Pinch
    float m_pinchAccum      = 1.0f;
    bool  m_pinchActive     = false;
    float m_pinchStartZoom  = 1.0f;

    // Visibility check debounce
    QTimer *m_visTimer      = nullptr;

    // Search
    QList<QList<QRectF>> m_matchRects;
    int m_totalMatches      = 0;

    QColor m_selColor       = QColor(77, 168, 255, 90);
};
