#include "PdfView.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QScrollBar>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QRegularExpression>
#include <QPropertyAnimation>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

// ════════════════════════════════════════════════════════════════════════════
// TextLayer
// ════════════════════════════════════════════════════════════════════════════

TextLayer::TextLayer(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::IBeamCursor);
    // Let mouse events through to parent when nothing is happening,
    // but capture them for selection
    setFocusPolicy(Qt::ClickFocus);
}

void TextLayer::setPageText(const PageText &chars, float zoom)
{
    m_zoom = zoom;
    m_chars.clear();
    // Scale bboxes from PDF points to widget pixels
    for (const TextChar &tc : chars) {
        TextChar scaled;
        scaled.bbox = QRectF(
            tc.bbox.x() * zoom,
            tc.bbox.y() * zoom,
            tc.bbox.width()  * zoom,
            tc.bbox.height() * zoom);
        scaled.ch = tc.ch;
        m_chars.append(scaled);
    }
    m_selIdx.clear();
    m_anchorIdx  = -1;
    m_currentIdx = -1;
    update();
}

void TextLayer::clear()
{
    m_chars.clear();
    m_selIdx.clear();
    m_anchorIdx  = -1;
    m_currentIdx = -1;
    update();
}

void TextLayer::selectAll()
{
    m_selIdx.clear();
    for (int i = 0; i < m_chars.size(); ++i)
        m_selIdx.append(i);
    emit selectionChanged();
    update();
}

QString TextLayer::selectedText() const
{
    if (m_selIdx.isEmpty()) return {};

    // Collect selected chars in order
    QList<const TextChar*> sel;
    for (int idx : m_selIdx) {
        if (idx < m_chars.size() && m_chars[idx].ch != "\n")
            sel.append(&m_chars[idx]);
    }
    if (sel.isEmpty()) return {};

    QString result;
    float prevBottom = sel.first()->bbox.bottom();
    float prevRight  = sel.first()->bbox.right();
    float lineHeight = sel.first()->bbox.height();

    for (const TextChar *tc : sel) {
        float top    = tc->bbox.top();
        float left   = tc->bbox.left();
        float height = tc->bbox.height();
        if (height > 0) lineHeight = height;

        // New line: vertical gap larger than half a line height
        float vertGap = top - prevBottom;
        if (vertGap > lineHeight * 0.3f) {
            result += "\n";
            prevRight = left;
        } else {
            // Same line: if there's a horizontal gap larger than ~half a space, add a space
            float hGap = left - prevRight;
            if (hGap > lineHeight * 0.25f && !result.endsWith(' ') && tc->ch != " ")
                result += " ";
        }

        if (tc->ch != " " || !result.endsWith(' '))
            result += tc->ch;

        prevBottom = tc->bbox.bottom();
        prevRight  = tc->bbox.right();
    }

    // Clean up: collapse multiple spaces, trim trailing spaces per line
    QStringList lines = result.split('\n');
    for (QString &line : lines) {
        // Collapse multiple spaces
        line.replace(QRegularExpression("  +"), " ");
        line = line.trimmed();
    }
    // Remove trailing empty lines
    while (!lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();

    return lines.join('\n');
}

int TextLayer::charAt(const QPoint &pt) const
{
    // Find the char whose bbox contains pt, with a small vertical tolerance
    for (int i = 0; i < m_chars.size(); ++i) {
        const QRectF &r = m_chars[i].bbox;
        // Expand bbox slightly for easier clicking
        QRectF expanded = r.adjusted(-1, -2, 1, 2);
        if (expanded.contains(pt)) return i;
    }
    // If no exact hit, find closest char on the same line (same y band)
    int best = -1;
    float bestDist = 1e9f;
    for (int i = 0; i < m_chars.size(); ++i) {
        const QRectF &r = m_chars[i].bbox;
        if (pt.y() >= r.top() - 2 && pt.y() <= r.bottom() + 2) {
            float dist = qAbs((float)pt.x() - (float)r.center().x());
            if (dist < bestDist) { bestDist = dist; best = i; }
        }
    }
    return best;
}

void TextLayer::updateSelection(int from, int to)
{
    if (from < 0 || to < 0) return;
    m_selIdx.clear();
    int start = qMin(from, to);
    int end   = qMax(from, to);
    for (int i = start; i <= end; ++i)
        m_selIdx.append(i);
    emit selectionChanged();
    update();
}

void TextLayer::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_pressing  = true;
        int idx     = charAt(e->pos());
        m_anchorIdx = idx;
        m_selIdx.clear();
        emit selectionChanged();
        update();
    }
}

void TextLayer::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_pressing) return;
    int idx = charAt(e->pos());
    if (idx >= 0 && idx != m_currentIdx) {
        m_currentIdx = idx;
        updateSelection(m_anchorIdx, m_currentIdx);
    }
}

void TextLayer::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_pressing = false;
        // Auto-copy to clipboard selection (like Linux convention)
        QString sel = selectedText();
        if (!sel.isEmpty())
            QApplication::clipboard()->setText(sel, QClipboard::Selection);
    }
}

void TextLayer::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    int idx = charAt(e->pos());
    if (idx < 0 || idx >= m_chars.size()) return;

    // Select the whole word around idx
    // Walk back to word start
    int start = idx;
    while (start > 0 && m_chars[start-1].ch != " " && m_chars[start-1].ch != "\n")
        --start;
    // Walk forward to word end
    int end = idx;
    while (end < m_chars.size()-1 && m_chars[end+1].ch != " " && m_chars[end+1].ch != "\n")
        ++end;
    updateSelection(start, end);
}

void TextLayer::paintEvent(QPaintEvent *)
{
    if (m_selIdx.isEmpty()) return;
    QPainter p(this);
    p.setPen(Qt::NoPen);

    // Build merged selection rects for cleaner rendering
    // Group consecutive chars per visual line, merge their bboxes
    QRectF lineRect;
    int    prevIdx = -2;
    for (int idx : m_selIdx) {
        if (idx >= m_chars.size()) continue;
        const QRectF &r = m_chars[idx].bbox;
        if (prevIdx < 0) {
            lineRect = r;
        } else if (idx == prevIdx + 1 &&
                   qAbs(r.top() - lineRect.top()) < r.height() * 0.5) {
            // Same line — extend rect
            lineRect = lineRect.united(r);
        } else {
            // New line — flush previous
            p.fillRect(lineRect.adjusted(-1,0,1,0), m_highlightColor);
            lineRect = r;
        }
        prevIdx = idx;
    }
    if (!lineRect.isNull())
        p.fillRect(lineRect.adjusted(-1,0,1,0), m_highlightColor);
}

// ════════════════════════════════════════════════════════════════════════════
// PageWidget
// ════════════════════════════════════════════════════════════════════════════

PageWidget::PageWidget(QWidget *parent) : QWidget(parent)
{
    setAutoFillBackground(false);
    m_textLayer = new TextLayer(this);
    m_textLayer->hide(); // shown after text is loaded
}

void PageWidget::setPixmap(const QPixmap &px)
{
    m_pixmap = px;
    resize(px.size());
    setMinimumSize(px.size());
    setMaximumSize(px.size());
    m_textLayer->resize(px.size());
    update();
}

void PageWidget::setPageText(const PageText &chars, float zoom)
{
    m_textLayer->setPageText(chars, zoom);
    m_textLayer->show();
    m_textLayer->raise();
}

void PageWidget::setHighlights(const QList<QRectF> &rects, float zoom)
{
    m_highlights.clear();
    for (const QRectF &r : rects)
        m_highlights.append(QRectF(r.x()*zoom, r.y()*zoom,
                                   r.width()*zoom, r.height()*zoom));
    update();
}

void PageWidget::setCurrentHighlight(int idx)
{
    m_currentHL = idx;
    update();
}

void PageWidget::clearHighlights()
{
    m_highlights.clear();
    m_currentHL = -1;
    update();
}

QString PageWidget::selectedText()  const { return m_textLayer->selectedText(); }
void    PageWidget::selectAll()           { m_textLayer->selectAll(); }
bool    PageWidget::hasSelection()  const { return m_textLayer->hasSelection(); }

void PageWidget::resizeEvent(QResizeEvent *e)
{
    m_textLayer->resize(e->size());
}

void PageWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_pixmap.isNull()) {
        p.fillRect(rect(), QColor(Theme::SURFACE2));
        p.setPen(QColor(Theme::TEXT_DIMMER));
        p.drawText(rect(), Qt::AlignCenter, "...");
        return;
    }
    p.drawPixmap(0, 0, m_pixmap);

    // Search highlights painted below text layer
    for (int i = 0; i < m_highlights.size(); ++i) {
        QColor c = (i == m_currentHL)
            ? QColor(77,168,255,120) : QColor(226,245,66,90);
        p.fillRect(m_highlights[i], c);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// PdfView
// ════════════════════════════════════════════════════════════════════════════

PdfView::PdfView(QWidget *parent) : QScrollArea(parent)
{
    setFrameShape(QFrame::NoFrame);
    setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    setStyleSheet(QString("QScrollArea{background:%1;border:none;}").arg(Theme::BG));

    m_inner = new QWidget(this);
    m_inner->setStyleSheet(QString("background:%1;").arg(Theme::BG));
    auto *lay = new QVBoxLayout(m_inner);
    lay->setContentsMargins(24,24,24,24);
    lay->setSpacing(16);
    lay->setAlignment(Qt::AlignHCenter);
    setWidget(m_inner);
    setWidgetResizable(true);

    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &PdfView::onScrollChanged);

    setFocusPolicy(Qt::StrongFocus);
}

void PdfView::setDocument(PdfDocument *doc)
{
    m_doc = doc;
    m_currentPage = 0;
    m_matchRects.clear();
    m_totalMatches = 0;
    buildPageWidgets();
}

void PdfView::buildPageWidgets()
{
    for (auto *pw : m_pages) pw->parentWidget()->deleteLater();
    m_pages.clear();

    auto *lay = qobject_cast<QVBoxLayout*>(m_inner->layout());
    while (lay->count()) delete lay->takeAt(0);

    if (!m_doc || !m_doc->isLoaded()) return;

    int total = m_doc->pageCount();
    m_pages.reserve(total);

    for (int i = 0; i < total; ++i) {
        auto *block = new QWidget(m_inner);
        block->setStyleSheet("background:transparent;");
        auto *blay = new QVBoxLayout(block);
        blay->setContentsMargins(0,0,0,0);
        blay->setSpacing(4);

        auto *pw = new PageWidget(block);
        pw->setStyleSheet("background:white;");
        // Forward selection changes from text layer
        connect(pw->textLayer(), &TextLayer::selectionChanged,
                this, [this]{ emit hasSelectionChanged(false); /* placeholder */ });

        auto *lbl = new QLabel(QString::number(i+1), block);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet(
            QString("color:%1;font-size:10px;background:transparent;")
            .arg(Theme::TEXT_DIMMER));

        blay->addWidget(pw, 0, Qt::AlignHCenter);
        blay->addWidget(lbl, 0, Qt::AlignHCenter);
        lay->addWidget(block, 0, Qt::AlignHCenter);
        m_pages.append(pw);
    }

    for (int i = 0; i < total; ++i) {
        renderPageAsync(i);
        loadTextAsync(i);
    }
}

void PdfView::renderPageAsync(int index)
{
    if (!m_doc) return;
    float zoom = m_zoom; int rotation = m_rotation; bool inv = m_inverted;
    auto *doc = m_doc;

    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished,
            this, [this, index, watcher]() {
        QImage img = watcher->result();
        if (!img.isNull() && index < m_pages.size()) {
            m_pages[index]->setPixmap(QPixmap::fromImage(img));
            if (index < m_matchRects.size() && !m_matchRects[index].isEmpty())
                m_pages[index]->setHighlights(m_matchRects[index], m_zoom);
            emit renderProgress(index+1, m_pages.size());
        }
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([doc,index,zoom,rotation,inv]()->QImage{
        QImage img = doc->renderPage(index, zoom, rotation);
        if (inv && !img.isNull()) img.invertPixels();
        return img;
    }));
}

void PdfView::loadTextAsync(int index)
{
    if (!m_doc) return;
    auto *doc = m_doc;
    float zoom = m_zoom;

    auto *watcher = new QFutureWatcher<PageText>(this);
    connect(watcher, &QFutureWatcher<PageText>::finished,
            this, [this, index, watcher]() {
        PageText text = watcher->result();
        if (index < m_pages.size())
            m_pages[index]->setPageText(text, m_zoom);
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([doc, index, zoom]()->PageText{
        Q_UNUSED(zoom)
        return doc->extractText(index);
    }));
}

void PdfView::rerender()
{
    if (!m_doc) return;
    for (int i = 0; i < m_pages.size(); ++i) {
        renderPageAsync(i);
        loadTextAsync(i);
    }
}

void PdfView::setZoom(float zoom)
{
    if (qAbs(zoom - m_zoom) < 0.001f) return;
    m_zoom = zoom;
    rerender();
}

void PdfView::setRotation(int deg)
{
    m_rotation = ((deg%360)+360)%360;
    rerender();
}

void PdfView::setInverted(bool inv)
{
    if (m_inverted == inv) return;
    m_inverted = inv;
    rerender();
}

void PdfView::scrollToPage(int index)
{
    if (index < 0 || index >= m_pages.size()) return;
    QWidget *block = m_pages[index]->parentWidget();
    if (block) verticalScrollBar()->setValue(block->mapTo(m_inner, QPoint(0,0)).y());
    m_currentPage = index;
    emit currentPageChanged(index+1);
}

int PdfView::pageCount() const { return m_doc ? m_doc->pageCount() : 0; }

// ── Text operations ───────────────────────────────────────────────────────────
QString PdfView::selectedText() const
{
    QString text;
    for (auto *pw : m_pages) {
        QString s = pw->selectedText();
        if (!s.isEmpty()) text += s;
    }
    return text;
}

void PdfView::copySelection() const
{
    QString text = selectedText();
    if (!text.isEmpty())
        QApplication::clipboard()->setText(text);
}

void PdfView::selectAll()
{
    // Select all on current visible page
    if (m_currentPage >= 0 && m_currentPage < m_pages.size())
        m_pages[m_currentPage]->selectAll();
}

void PdfView::setSelectionColor(const QColor &c)
{
    for (auto *pw : m_pages)
        pw->textLayer()->setHighlightColor(c);
}

// ── Keyboard ──────────────────────────────────────────────────────────────────
void PdfView::keyPressEvent(QKeyEvent *e)
{
    if ((e->modifiers() & Qt::ControlModifier)) {
        if (e->key() == Qt::Key_C) { copySelection(); return; }
        if (e->key() == Qt::Key_A) { selectAll();     return; }
    }
    QScrollArea::keyPressEvent(e);
}

// ── Ctrl+scroll zoom ─────────────────────────────────────────────────────────
void PdfView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        e->accept();
        float delta = e->angleDelta().y();
        if (qAbs(delta) < 1.0f) return;
        float factor  = (delta > 0) ? 1.10f : (1.0f/1.10f);
        float newZoom = qBound(0.1f, m_zoom * factor, 6.0f);
        if (qAbs(newZoom - m_zoom) > 0.005f)
            emit zoomChangeRequested(newZoom);
    } else {
        QScrollArea::wheelEvent(e);
    }
}

void PdfView::onScrollChanged()
{
    if (m_pages.isEmpty()) return;
    int vpTop = verticalScrollBar()->value();
    int best = 0, bestDist = INT_MAX;
    for (int i = 0; i < m_pages.size(); ++i) {
        QWidget *block = m_pages[i]->parentWidget();
        if (!block) continue;
        int dist = qAbs(block->mapTo(m_inner, QPoint(0,0)).y() - vpTop);
        if (dist < bestDist) { bestDist = dist; best = i; }
    }
    if (best != m_currentPage) {
        m_currentPage = best;
        emit currentPageChanged(best+1);
    }
}

void PdfView::setSearchMatches(const QList<SearchMatch> &matches)
{
    clearSearchMatches();
    if (!m_doc) return;
    int total = m_doc->pageCount();
    m_matchRects.resize(total);
    m_totalMatches = 0;
    for (const SearchMatch &sm : matches) {
        if (sm.pageIndex < total) {
            m_matchRects[sm.pageIndex] = sm.rects;
            m_totalMatches += sm.rects.size();
        }
    }
    for (int i = 0; i < m_pages.size(); ++i)
        if (i < m_matchRects.size() && !m_matchRects[i].isEmpty())
            m_pages[i]->setHighlights(m_matchRects[i], m_zoom);
}

void PdfView::clearSearchMatches()
{
    m_matchRects.clear();
    m_totalMatches = 0;
    for (auto *pw : m_pages) pw->clearHighlights();
}

void PdfView::jumpToMatch(int globalIdx)
{
    int count = 0;
    for (int p = 0; p < m_matchRects.size(); ++p) {
        int n = m_matchRects[p].size();
        if (globalIdx < count + n) {
            int localIdx = globalIdx - count;

            if (p < m_pages.size())
                m_pages[p]->setCurrentHighlight(localIdx);

            QWidget *block = (p < m_pages.size()) ? m_pages[p]->parentWidget() : nullptr;
            if (!block) { scrollToPage(p); return; }

            int blockY     = block->mapTo(m_inner, QPoint(0,0)).y();
            const QRectF &r = m_matchRects[p][localIdx];
            int matchY     = (int)(r.top() * m_zoom);
            int matchH     = (int)(r.height() * m_zoom);
            int absMatchY  = blockY + matchY;
            int viewportH  = viewport()->height();

            // Center the match vertically in the viewport
            int scrollTo   = absMatchY - (viewportH - matchH) / 2;
            scrollTo       = qMax(0, qMin(scrollTo, verticalScrollBar()->maximum()));

            // Smooth animation
            auto *anim = new QPropertyAnimation(verticalScrollBar(), "value", this);
            anim->setDuration(180);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setStartValue(verticalScrollBar()->value());
            anim->setEndValue(scrollTo);
            anim->start(QAbstractAnimation::DeleteWhenStopped);

            m_currentPage = p;
            emit currentPageChanged(p + 1);
            return;
        }
        count += n;
    }
}
