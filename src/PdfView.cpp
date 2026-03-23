#include "PdfView.h"
#include "Theme.h"
#include <QPainter>
#include <QScrollBar>
#include <QApplication>
#include <QClipboard>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QNativeGestureEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QRegularExpression>
#include <QPropertyAnimation>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QCursor>
#include <cmath>

// ════════════════════════════════════════════════════════════════════════════
// PdfPageItem
// ════════════════════════════════════════════════════════════════════════════

PdfPageItem::PdfPageItem(PdfDocument *doc, int pageIndex,
                         float zoom, int rotation, bool inverted,
                         QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_doc(doc)
    , m_pageIndex(pageIndex)
{
    setAcceptHoverEvents(true);

    // Calcola dimensione dalla pageInfo
    PageInfo info = doc->pageInfo(pageIndex);
    float w = info.sizePt.width()  * zoom;
    float h = info.sizePt.height() * zoom;
    if (rotation == 90 || rotation == 270) std::swap(w, h);
    m_size = QSizeF(qMax(1.0f, w), qMax(1.0f, h));
}

QRectF PdfPageItem::boundingRect() const
{
    return QRectF(0, 0, m_size.width(), m_size.height());
}

void PdfPageItem::paint(QPainter *painter,
                        const QStyleOptionGraphicsItem *, QWidget *)
{
    // Sfondo bianco pagina
    painter->fillRect(boundingRect(), Qt::white);

    if (!m_pixmap.isNull()) {
        painter->drawPixmap(0, 0, (int)m_size.width(), (int)m_size.height(),
                            m_pixmap);
    } else {
        // Placeholder grigio con numero pagina
        painter->setPen(QColor(Theme::TEXT_DIMMER));
        painter->setFont(QFont("sans-serif", 12));
        painter->drawText(boundingRect(), Qt::AlignCenter,
                          QString("Pag. %1").arg(m_pageIndex + 1));
    }

    // Search highlights
    for (int i = 0; i < m_highlights.size(); ++i) {
        QColor c = (i == m_currentHL)
            ? QColor(77, 168, 255, 140) : QColor(226, 245, 66, 100);
        painter->fillRect(m_highlights[i], c);
    }

    // Text selection
    if (!m_selIdx.isEmpty()) {
        QRectF lineRect;
        int prevIdx = -2;
        for (int idx : m_selIdx) {
            if (idx >= m_chars.size()) continue;
            const QRectF &r = m_chars[idx].bbox;
            if (prevIdx < 0) {
                lineRect = r;
            } else if (idx == prevIdx + 1 &&
                       std::abs(r.top() - lineRect.top()) < r.height() * 0.5) {
                lineRect = lineRect.united(r);
            } else {
                painter->fillRect(lineRect.adjusted(-1,0,1,0), m_selColor);
                lineRect = r;
            }
            prevIdx = idx;
        }
        if (!lineRect.isNull())
            painter->fillRect(lineRect.adjusted(-1,0,1,0), m_selColor);
    }

    // Bordo pagina
    painter->setPen(QPen(QColor(Theme::BORDER), 0.5));
    painter->drawRect(boundingRect().adjusted(0,0,-0.5,-0.5));
}

void PdfPageItem::requestRender(float zoom, int rotation, bool inverted)
{
    if (m_renderPending) return;
    if (!m_pixmap.isNull() &&
        qAbs(m_renderedZoom - zoom) < 0.001f &&
        m_renderedRotation == rotation &&
        m_renderedInverted == inverted) return;

    m_renderPending = true;
    auto *doc = m_doc;
    int idx   = m_pageIndex;

    auto *watcher = new QFutureWatcher<QImage>();
    QObject::connect(watcher, &QFutureWatcher<QImage>::finished,
                     this, [this, watcher, zoom, rotation, inverted]() {
        QImage img = watcher->result();
        watcher->deleteLater();
        m_renderPending = false;
        if (!img.isNull()) {
            m_pixmap          = QPixmap::fromImage(img);
            m_renderedZoom    = zoom;
            m_renderedRotation = rotation;
            m_renderedInverted = inverted;
            // Aggiorna dimensione reale
            m_size = QSizeF(img.width(), img.height());
            prepareGeometryChange();
            update();
            emit rendered(m_pageIndex);
        }
    });
    watcher->setFuture(QtConcurrent::run([doc, idx, zoom, rotation, inverted]() -> QImage {
        QImage img = doc->renderPage(idx, zoom, rotation);
        if (inverted && !img.isNull()) img.invertPixels();
        return img;
    }));
}

// ── Text selection ────────────────────────────────────────────────────────────
void PdfPageItem::setPageText(const PageText &chars, float zoom)
{
    m_chars.clear();
    for (const TextChar &tc : chars) {
        TextChar scaled;
        scaled.bbox = QRectF(tc.bbox.x() * zoom, tc.bbox.y() * zoom,
                             tc.bbox.width() * zoom, tc.bbox.height() * zoom);
        scaled.ch = tc.ch;
        m_chars.append(scaled);
    }
    m_selIdx.clear();
    m_anchorIdx = -1;
    update();
}

QString PdfPageItem::selectedText() const
{
    if (m_selIdx.isEmpty()) return {};
    QList<const TextChar*> sel;
    for (int idx : m_selIdx)
        if (idx < m_chars.size() && m_chars[idx].ch != "\n")
            sel.append(&m_chars[idx]);
    if (sel.isEmpty()) return {};

    QString result;
    float prevBottom = sel.first()->bbox.bottom();
    float prevRight  = sel.first()->bbox.right();
    float lineH      = sel.first()->bbox.height();

    for (const TextChar *tc : sel) {
        if (tc->bbox.height() > 0) lineH = tc->bbox.height();
        float vertGap = tc->bbox.top() - prevBottom;
        if (vertGap > lineH * 0.3f) {
            result += "\n";
        } else {
            float hGap = tc->bbox.left() - prevRight;
            if (hGap > lineH * 0.25f && !result.endsWith(' ') && tc->ch != " ")
                result += " ";
        }
        if (tc->ch != " " || !result.endsWith(' '))
            result += tc->ch;
        prevBottom = tc->bbox.bottom();
        prevRight  = tc->bbox.right();
    }
    QStringList lines = result.split('\n');
    for (QString &line : lines) {
        line.replace(QRegularExpression("  +"), " ");
        line = line.trimmed();
    }
    while (!lines.isEmpty() && lines.last().isEmpty()) lines.removeLast();
    return lines.join('\n');
}

void PdfPageItem::selectAll()
{
    m_selIdx.clear();
    for (int i = 0; i < m_chars.size(); ++i) m_selIdx.append(i);
    update();
}

void PdfPageItem::clearSelection()
{
    m_selIdx.clear();
    m_anchorIdx = -1;
    m_pressing  = false;
    update();
}

int PdfPageItem::charAt(const QPointF &pt) const
{
    for (int i = 0; i < m_chars.size(); ++i) {
        if (m_chars[i].bbox.adjusted(-1,-2,1,2).contains(pt)) return i;
    }
    int best = -1; float bestDist = 1e9f;
    for (int i = 0; i < m_chars.size(); ++i) {
        const QRectF &r = m_chars[i].bbox;
        if (pt.y() >= r.top()-2 && pt.y() <= r.bottom()+2) {
            float dist = std::abs((float)pt.x() - (float)r.center().x());
            if (dist < bestDist) { bestDist = dist; best = i; }
        }
    }
    return best;
}

void PdfPageItem::updateSelection(int from, int to)
{
    if (from < 0 || to < 0) return;
    m_selIdx.clear();
    int s = qMin(from,to), e = qMax(from,to);
    for (int i = s; i <= e; ++i) m_selIdx.append(i);
    update();
}

void PdfPageItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_pressing  = true;
        m_anchorIdx = charAt(e->pos());
        m_selIdx.clear();
        update();
    }
}

void PdfPageItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    if (!m_pressing) return;
    int idx = charAt(e->pos());
    if (idx >= 0) updateSelection(m_anchorIdx, idx);
}

void PdfPageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_pressing = false;
        QString sel = selectedText();
        if (!sel.isEmpty())
            QApplication::clipboard()->setText(sel, QClipboard::Selection);
    }
}

void PdfPageItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    int idx = charAt(e->pos());
    if (idx < 0 || idx >= m_chars.size()) return;
    int start = idx;
    while (start > 0 && m_chars[start-1].ch != " " && m_chars[start-1].ch != "\n") --start;
    int end = idx;
    while (end < m_chars.size()-1 && m_chars[end+1].ch != " " && m_chars[end+1].ch != "\n") ++end;
    updateSelection(start, end);
}

void PdfPageItem::hoverMoveEvent(QGraphicsSceneHoverEvent *)
{
    if (!m_chars.isEmpty()) scene()->views().first()->setCursor(Qt::IBeamCursor);
    else scene()->views().first()->setCursor(Qt::ArrowCursor);
}

// ── Highlights ────────────────────────────────────────────────────────────────
void PdfPageItem::setHighlights(const QList<QRectF> &rects, float zoom)
{
    m_highlights.clear();
    for (const QRectF &r : rects)
        m_highlights.append(QRectF(r.x()*zoom, r.y()*zoom,
                                   r.width()*zoom, r.height()*zoom));
    update();
}

void PdfPageItem::setCurrentHighlight(int idx) { m_currentHL = idx; update(); }
void PdfPageItem::clearHighlights() { m_highlights.clear(); m_currentHL = -1; update(); }

// ════════════════════════════════════════════════════════════════════════════
// PdfScene
// ════════════════════════════════════════════════════════════════════════════

PdfScene::PdfScene(QObject *parent) : QGraphicsScene(parent) {}

void PdfScene::setDocument(PdfDocument *doc, float zoom, int rotation,
                            bool inverted, bool doublePage)
{
    QGraphicsScene::clear();
    m_items.clear();
    m_doc       = doc;
    m_zoom      = zoom;
    m_rotation  = rotation;
    m_inverted  = inverted;
    m_doublePage = doublePage;
    if (!doc || !doc->isLoaded()) return;

    int total = doc->pageCount();

    if (doublePage) {
        // Modalità doppia pagina: coppie affiancate
        float y = (float)PAGE_GAP;
        for (int i = 0; i < total; i += 2) {
            auto *left  = new PdfPageItem(doc, i, zoom, rotation, inverted);
            left->setSelectionColor(m_selColor);
            addItem(left);
            m_items.append(left);
            connect(left, &PdfPageItem::rendered, this, [this](int idx){
                emit renderProgress(idx+1, m_items.size());
            });

            float rowH = left->boundingRect().height();
            float rowW = left->boundingRect().width();

            if (i+1 < total) {
                auto *right = new PdfPageItem(doc, i+1, zoom, rotation, inverted);
                right->setSelectionColor(m_selColor);
                addItem(right);
                m_items.append(right);
                connect(right, &PdfPageItem::rendered, this, [this](int idx){
                    emit renderProgress(idx+1, m_items.size());
                });
                // Posiziona destra accanto a sinistra
                left->setPos(PAGE_GAP, y);
                right->setPos(PAGE_GAP + rowW + PAGE_GAP, y);
                rowH = qMax(rowH, (float)right->boundingRect().height());
                rowW = rowW + PAGE_GAP + right->boundingRect().width();
            } else {
                left->setPos(PAGE_GAP, y);
            }
            y += rowH + PAGE_GAP;
        }
        // Centra
        float maxW = 0;
        for (auto *item : m_items)
            maxW = qMax(maxW, (float)(item->pos().x() + item->boundingRect().width()));
        setSceneRect(0, 0, maxW + PAGE_GAP, y);
    } else {
        // Modalità singola pagina
        float y = (float)PAGE_GAP;
        float maxW = 0;
        for (int i = 0; i < total; ++i) {
            auto *item = new PdfPageItem(doc, i, zoom, rotation, inverted);
            item->setPos(PAGE_GAP, y);
            item->setSelectionColor(m_selColor);
            addItem(item);
            m_items.append(item);
            connect(item, &PdfPageItem::rendered, this, [this](int idx){
                emit renderProgress(idx+1, m_items.size());
            });
            maxW = qMax(maxW, (float)item->boundingRect().width());
            y += item->boundingRect().height() + PAGE_GAP;
        }
        // Centra orizzontalmente
        for (auto *item : m_items)
            item->setX((maxW - item->boundingRect().width()) / 2.0f + PAGE_GAP);
        setSceneRect(0, 0, maxW + PAGE_GAP*2, y);
    }
}

void PdfScene::clear()
{
    QGraphicsScene::clear();
    m_items.clear();
}

PdfPageItem *PdfScene::pageItem(int index) const
{
    if (index < 0 || index >= m_items.size()) return nullptr;
    return m_items[index];
}

QRectF PdfScene::pageRect(int index) const
{
    auto *item = pageItem(index);
    if (!item) return {};
    return QRectF(item->pos(), item->boundingRect().size());
}

void PdfScene::setZoom(float zoom)
{
    m_zoom = zoom;
    if (m_doc) setDocument(m_doc, zoom, m_rotation, m_inverted, m_doublePage);
}

void PdfScene::setRotation(int deg)
{
    m_rotation = deg;
    if (m_doc) setDocument(m_doc, m_zoom, deg, m_inverted, m_doublePage);
}

void PdfScene::setInverted(bool inv)
{
    m_inverted = inv;
    if (m_doc) setDocument(m_doc, m_zoom, m_rotation, inv, m_doublePage);
}

void PdfScene::setSearchMatches(const QList<SearchMatch> &matches)
{
    clearSearchMatches();
    m_totalMatches = 0;
    for (const SearchMatch &sm : matches) {
        if (sm.pageIndex < m_items.size()) {
            m_items[sm.pageIndex]->setHighlights(sm.rects, m_zoom);
            m_totalMatches += sm.rects.size();
        }
    }
}

void PdfScene::clearSearchMatches()
{
    for (auto *item : m_items) item->clearHighlights();
    m_totalMatches = 0;
}

void PdfScene::setCurrentHighlight(int globalIdx)
{
    int count = 0;
    for (auto *item : m_items) {
        // reset current on all
        item->setCurrentHighlight(-1);
    }
    count = 0;
    for (auto *item : m_items) {
        int n = item->boundingRect().isEmpty() ? 0 : 0;
        Q_UNUSED(n);
        // Recount via match rects stored in item
        // We'll handle this in PdfView::jumpToMatch
        Q_UNUSED(globalIdx); Q_UNUSED(count);
        break;
    }
}

void PdfScene::setSelectionColor(const QColor &c)
{
    m_selColor = c;
    for (auto *item : m_items) item->setSelectionColor(c);
}

QString PdfScene::selectedText() const
{
    QString text;
    for (auto *item : m_items) {
        QString s = item->selectedText();
        if (!s.isEmpty()) text += s;
    }
    return text;
}

void PdfScene::selectAll(int pageIndex)
{
    if (pageIndex >= 0 && pageIndex < m_items.size())
        m_items[pageIndex]->selectAll();
}

// ════════════════════════════════════════════════════════════════════════════
// PdfView
// ════════════════════════════════════════════════════════════════════════════

PdfView::PdfView(QWidget *parent) : QGraphicsView(parent)
{
    m_scene = new PdfScene(this);
    setScene(m_scene);

    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(QColor(Theme::BG));
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    setOptimizationFlag(QGraphicsView::DontSavePainterState);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

    // Debounce timer per il controllo visibilità
    m_visTimer = new QTimer(this);
    m_visTimer->setSingleShot(true);
    m_visTimer->setInterval(50);
    connect(m_visTimer, &QTimer::timeout, this, &PdfView::onVisibilityCheck);

    connect(m_scene, &PdfScene::renderProgress, this, &PdfView::renderProgress);

    setFocusPolicy(Qt::StrongFocus);
    viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
}

PdfView::~PdfView() {}

void PdfView::setDocument(PdfDocument *doc)
{
    m_doc = doc;
    m_currentPage = 0;
    m_matchRects.clear();
    m_totalMatches = 0;
    m_scene->setDocument(doc, m_zoom, m_rotation, m_inverted, m_doublePage);
    applyZoomTransform();
    m_visTimer->start();
}

void PdfView::applyZoomTransform()
{
    // Con QGraphicsView lo zoom si applica come trasformazione della vista
    // Le coordinate della scena sono in pixel @zoom=1, la view scala tutto
    setTransform(QTransform::fromScale(1.0, 1.0));
}

void PdfView::rerender()
{
    if (!m_doc) return;
    m_scene->setDocument(m_doc, m_zoom, m_rotation, m_inverted, m_doublePage);
    m_visTimer->start();
}

// ── Zoom ──────────────────────────────────────────────────────────────────────
void PdfView::setZoom(float z)
{
    if (qAbs(z - m_zoom) < 0.001f) return;
    m_zoom = z;
    // Ricrea la scena al nuovo zoom (le pagine vengono ri-renderizzate alla nuova risoluzione)
    m_scene->setZoom(z);
    m_visTimer->start();
}

void PdfView::setRotation(int deg)
{
    m_rotation = ((deg%360)+360)%360;
    m_scene->setRotation(m_rotation);
    m_visTimer->start();
}

void PdfView::setInverted(bool inv)
{
    if (m_inverted == inv) return;
    m_inverted = inv;
    m_scene->setInverted(inv);
    m_visTimer->start();
}

void PdfView::setDoublePageMode(bool on)
{
    m_doublePage = on;
    rerender();
}

// ── Navigation ────────────────────────────────────────────────────────────────
void PdfView::scrollToPage(int index)
{
    if (!m_scene || index < 0 || index >= m_scene->pageCount()) return;
    QRectF r = m_scene->pageRect(index);
    // Scroll animato alla pagina
    auto *anim = new QPropertyAnimation(verticalScrollBar(), "value", this);
    anim->setDuration(200);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setStartValue(verticalScrollBar()->value());
    int target = (int)(r.top() - 20);
    anim->setEndValue(qMax(0, target));
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    m_currentPage = index;
    emit currentPageChanged(index + 1);
}

int PdfView::pageCount() const
{
    return m_scene ? m_scene->pageCount() : 0;
}

// ── Visibility check — lazy rendering ─────────────────────────────────────────
void PdfView::onVisibilityCheck()
{
    checkVisiblePages();
}

void PdfView::checkVisiblePages()
{
    if (!m_scene || !m_doc) return;
    constexpr int BUFFER = 2;

    QRectF visible = mapToScene(viewport()->rect()).boundingRect();
    visible.adjust(0, -viewport()->height()*BUFFER, 0, viewport()->height()*BUFFER);

    int total = m_scene->pageCount();
    for (int i = 0; i < total; ++i) {
        auto *item = m_scene->pageItem(i);
        if (!item) continue;
        QRectF itemRect = item->mapToScene(item->boundingRect()).boundingRect();
        if (itemRect.intersects(visible)) {
            if (!item->isRendered()) {
                item->requestRender(m_zoom, m_rotation, m_inverted);
                // Carica anche il testo
                auto *doc = m_doc;
                int idx   = i;
                float zoom = m_zoom;
                auto *watcher = new QFutureWatcher<PageText>(this);
                connect(watcher, &QFutureWatcher<PageText>::finished,
                        this, [this, idx, watcher, zoom]() {
                    PageText text = watcher->result();
                    watcher->deleteLater();
                    auto *it = m_scene->pageItem(idx);
                    if (it) it->setPageText(text, zoom);
                });
                watcher->setFuture(QtConcurrent::run([doc, idx]() -> PageText {
                    return doc->extractText(idx);
                }));
            }
        }
    }

    // Aggiorna pagina corrente
    QPointF center = mapToScene(viewport()->rect().center());
    int best = 0; float bestDist = 1e9f;
    for (int i = 0; i < total; ++i) {
        auto *item = m_scene->pageItem(i);
        if (!item) continue;
        float dist = std::abs((float)(item->pos().y() + item->boundingRect().height()/2) - (float)center.y());
        if (dist < bestDist) { bestDist = dist; best = i; }
    }
    if (best != m_currentPage) {
        m_currentPage = best;
        emit currentPageChanged(best + 1);
    }
}

void PdfView::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    m_visTimer->start();
}

void PdfView::resizeEvent(QResizeEvent *e)
{
    QGraphicsView::resizeEvent(e);
    m_visTimer->start();
}

// ── Wheel zoom ────────────────────────────────────────────────────────────────
void PdfView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        e->accept();
        float delta = e->angleDelta().y();
        if (qAbs(delta) < 1.0f) return;
        float factor  = (delta > 0) ? 1.12f : (1.0f/1.12f);
        float newZoom = qBound(0.1f, m_zoom * factor, 6.0f);
        if (qAbs(newZoom - m_zoom) > 0.005f)
            emit zoomChangeRequested(newZoom);
    } else {
        QGraphicsView::wheelEvent(e);
    }
}

// ── Pinch zoom — QGraphicsView handles this natively via scale transform ───────
bool PdfView::event(QEvent *e)
{
    if (e->type() == QEvent::NativeGesture) {
        auto *ge = static_cast<QNativeGestureEvent*>(e);

        if (ge->gestureType() == Qt::BeginNativeGesture) {
            m_pinchActive    = true;
            m_pinchAccum     = 1.0f;
            m_pinchStartZoom = m_zoom;
            e->accept();
            return true;
        }

        if (ge->gestureType() == Qt::ZoomNativeGesture && m_pinchActive) {
            m_pinchAccum *= (1.0f + (float)ge->value());

            // Zoom visivo istantaneo via trasformazione QGraphicsView
            // NON re-renderizza — scala solo la vista
            float visualZoom = qBound(0.05f, m_pinchStartZoom * m_pinchAccum, 12.0f);
            float scale = visualZoom / m_pinchStartZoom;
            setTransform(QTransform::fromScale(scale, scale));

            e->accept();
            return true;
        }

        if (ge->gestureType() == Qt::EndNativeGesture && m_pinchActive) {
            m_pinchActive = false;
            float finalZoom = qBound(0.1f, m_pinchStartZoom * m_pinchAccum, 6.0f);
            m_pinchAccum = 1.0f;

            // Reimposta la trasformazione e re-renderizza alla nuova risoluzione
            setTransform(QTransform::fromScale(1.0, 1.0));
            emit zoomChangeRequested(finalZoom);
            e->accept();
            return true;
        }
    }
    return QGraphicsView::event(e);
}

// ── Keyboard ──────────────────────────────────────────────────────────────────
void PdfView::keyPressEvent(QKeyEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->key() == Qt::Key_C) { copySelection(); return; }
        if (e->key() == Qt::Key_A) { selectAll();     return; }
    }
    QGraphicsView::keyPressEvent(e);
}

// ── Search ────────────────────────────────────────────────────────────────────
void PdfView::setSearchMatches(const QList<SearchMatch> &matches)
{
    clearSearchMatches();
    if (!m_doc) return;
    int total = m_scene->pageCount();
    m_matchRects.resize(total);
    m_totalMatches = 0;
    for (const SearchMatch &sm : matches) {
        if (sm.pageIndex < total) {
            m_matchRects[sm.pageIndex] = sm.rects;
            m_totalMatches += sm.rects.size();
        }
    }
    m_scene->setSearchMatches(matches);
}

void PdfView::clearSearchMatches()
{
    m_matchRects.clear();
    m_totalMatches = 0;
    m_scene->clearSearchMatches();
}

int PdfView::totalMatchCount() const { return m_totalMatches; }

void PdfView::jumpToMatch(int globalIdx)
{
    int count = 0;
    for (int p = 0; p < m_matchRects.size(); ++p) {
        int n = m_matchRects[p].size();
        if (globalIdx < count + n) {
            int localIdx = globalIdx - count;
            auto *item = m_scene->pageItem(p);
            if (item) item->setCurrentHighlight(localIdx);

            // Scroll animato alla posizione del match
            QRectF r = m_scene->pageRect(p);
            float matchY = r.top() + m_matchRects[p][localIdx].top() * m_zoom;
            int   target = (int)(matchY - viewport()->height() / 2);

            auto *anim = new QPropertyAnimation(verticalScrollBar(), "value", this);
            anim->setDuration(180);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setStartValue(verticalScrollBar()->value());
            anim->setEndValue(qBound(0, target, verticalScrollBar()->maximum()));
            anim->start(QAbstractAnimation::DeleteWhenStopped);

            m_currentPage = p;
            emit currentPageChanged(p + 1);
            return;
        }
        count += n;
    }
}

// ── Text operations ───────────────────────────────────────────────────────────
QString PdfView::selectedText() const
{
    return m_scene ? m_scene->selectedText() : QString();
}

void PdfView::copySelection() const
{
    QString text = selectedText();
    if (!text.isEmpty()) QApplication::clipboard()->setText(text);
}

void PdfView::selectAll()
{
    if (m_scene) m_scene->selectAll(m_currentPage);
}

void PdfView::setSelectionColor(const QColor &c)
{
    m_selColor = c;
    if (m_scene) m_scene->setSelectionColor(c);
}
