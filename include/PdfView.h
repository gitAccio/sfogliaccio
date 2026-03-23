#pragma once
#include <QScrollArea>
#include <QWidget>
#include <QList>
#include <QPixmap>
#include <QRectF>
#include <QPoint>
#include <QWheelEvent>
#include "PdfDocument.h"

// ── TextLayer ─────────────────────────────────────────────────────────────────
// Transparent overlay sitting exactly on top of a rendered page.
// Handles mouse selection and maps pixel coords → PDF chars.
class TextLayer : public QWidget
{
    Q_OBJECT
public:
    explicit TextLayer(QWidget *parent = nullptr);

    // Feed the extracted chars and the current zoom so we can map coords
    void setPageText(const PageText &chars, float zoom);
    void clear();

    // Select all text on this page
    void selectAll();

    // Return the currently selected text
    QString selectedText() const;

    // You can set a custom highlight color (for annotation-style highlighting)
    void setHighlightColor(const QColor &c) { m_highlightColor = c; update(); }
    QColor highlightColor() const { return m_highlightColor; }

    // True if any text is selected
    bool hasSelection() const { return !m_selIdx.isEmpty(); }

signals:
    void selectionChanged();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

private:
    // Return the index of the char whose bbox contains pt, or -1
    int charAt(const QPoint &pt) const;
    // Return indices of all chars between anchor and current
    void updateSelection(int from, int to);

    PageText      m_chars;   // chars with bbox already scaled to widget coords
    QList<int>    m_selIdx;  // selected char indices, in order

    int  m_anchorIdx  = -1;
    int  m_currentIdx = -1;
    bool m_pressing   = false;
    float m_zoom      = 1.0f;
    QColor m_highlightColor = QColor(77, 168, 255, 90); // default blue selection
};

// ── PageWidget ────────────────────────────────────────────────────────────────
class PageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PageWidget(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &px);
    void setPageText(const PageText &chars, float zoom);
    void setHighlights(const QList<QRectF> &rects, float zoom);
    void setCurrentHighlight(int idx);
    void clearHighlights();

    TextLayer *textLayer() { return m_textLayer; }

    // Proxy for copy
    QString selectedText() const;
    void    selectAll();
    bool    hasSelection() const;

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    QPixmap       m_pixmap;
    QList<QRectF> m_highlights;
    int           m_currentHL  = -1;
    TextLayer    *m_textLayer  = nullptr;
};

// ── PdfView ───────────────────────────────────────────────────────────────────
class PdfView : public QScrollArea
{
    Q_OBJECT
public:
    explicit PdfView(QWidget *parent = nullptr);

    void setDocument(PdfDocument *doc);
    void rerender();

    void  setZoom(float z);
    float zoom()     const { return m_zoom; }
    void  setRotation(int deg);
    int   rotation() const { return m_rotation; }
    void  setInverted(bool inv);
    bool  inverted() const { return m_inverted; }

    void scrollToPage(int index);
    int  currentPage() const { return m_currentPage; }
    int  pageCount()   const;

    void setSearchMatches(const QList<SearchMatch> &matches);
    void clearSearchMatches();
    void jumpToMatch(int globalIdx);
    int  totalMatchCount() const { return m_totalMatches; }

    // Text operations
    QString selectedText() const;
    void    copySelection() const;
    void    selectAll();
    void    setSelectionColor(const QColor &c);

signals:
    void currentPageChanged(int page); // 1-based
    void renderProgress(int done, int total);
    void zoomChangeRequested(float newZoom);
    void hasSelectionChanged(bool has);

protected:
    void wheelEvent(QWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void onScrollChanged();

private:
    void buildPageWidgets();
    void renderPageAsync(int index);
    void loadTextAsync(int index);

    PdfDocument         *m_doc    = nullptr;
    QWidget             *m_inner  = nullptr;
    QList<PageWidget *>  m_pages;

    float m_zoom         = 1.0f;
    int   m_rotation     = 0;
    bool  m_inverted     = false;
    int   m_currentPage  = 0;
    int   m_totalMatches = 0;

    QList<QList<QRectF>> m_matchRects;
};
