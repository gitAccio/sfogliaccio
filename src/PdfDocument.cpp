#include "PdfDocument.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrent>

// ── MuPDF version compatibility ───────────────────────────────────────────────
#ifndef FZ_VERSION_MAJOR
#  define FZ_VERSION_MAJOR 1
#endif
#ifndef FZ_VERSION_MINOR
#  define FZ_VERSION_MINOR 0
#endif
#define MUPDF_VERSION_GE(maj, min) \
    (FZ_VERSION_MAJOR > (maj) || (FZ_VERSION_MAJOR == (maj) && FZ_VERSION_MINOR >= (min)))

#if MUPDF_VERSION_GE(1, 22)
#  define MUPDF_SEARCH(ctx, pg, needle, quads, max) \
     fz_search_page(ctx, pg, needle, nullptr, quads, max)
#else
#  define MUPDF_SEARCH(ctx, pg, needle, quads, max) \
     fz_search_page(ctx, pg, needle, quads, max)
#endif

#if MUPDF_VERSION_GE(1, 21)
#  define MUPDF_OUTLINE_PAGE(n) ((n)->page.page)
#else
#  define MUPDF_OUTLINE_PAGE(n) ((n)->page)
#endif

// ── ctor/dtor ─────────────────────────────────────────────────────────────────
PdfDocument::PdfDocument(QObject *parent) : QObject(parent)
{
    m_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!m_ctx) { emit loadError("Impossibile creare contesto MuPDF"); return; }
    fz_register_document_handlers(m_ctx);
}

PdfDocument::~PdfDocument()
{
    cancelSearch();
    if (m_doc && m_ctx) fz_drop_document(m_ctx, m_doc);
    if (m_ctx)          fz_drop_context(m_ctx);
}

// ── load ──────────────────────────────────────────────────────────────────────
bool PdfDocument::load(const QString &path)
{
    QMutexLocker lock(&m_mutex);
    if (m_doc) { fz_drop_document(m_ctx, m_doc); m_doc = nullptr; }
    m_loaded = false; m_pageCount = 0; m_path.clear();
    m_needsPassword = false;

    fz_try(m_ctx) {
        m_doc = fz_open_document(m_ctx, path.toUtf8().constData());

        // Controlla se richiede password
        if (fz_needs_password(m_ctx, m_doc)) {
            m_needsPassword = true;
            m_path = path;
            // Non droppare m_doc — lo teniamo per loadWithPassword
            emit passwordRequired();
            return false;
        }

        m_pageCount = fz_count_pages(m_ctx, m_doc);
        m_path      = path;
        m_loaded    = true;
    }
    fz_catch(m_ctx) {
        emit loadError(QString("Errore: %1").arg(fz_caught_message(m_ctx)));
        return false;
    }
    return true;
}

bool PdfDocument::loadWithPassword(const QString &path, const QString &password)
{
    QMutexLocker lock(&m_mutex);

    // Se il doc è già aperto (da load() che ha trovato password), usa quello
    if (!m_doc) {
        fz_try(m_ctx) {
            m_doc = fz_open_document(m_ctx, path.toUtf8().constData());
        }
        fz_catch(m_ctx) {
            emit loadError(QString("Errore: %1").arg(fz_caught_message(m_ctx)));
            return false;
        }
    }

    bool ok = false;
    fz_try(m_ctx) {
        ok = fz_authenticate_password(m_ctx, m_doc,
                                       password.toUtf8().constData());
    }
    fz_catch(m_ctx) {}

    if (!ok) {
        emit loadError("Password errata.");
        return false;
    }

    fz_try(m_ctx) {
        m_pageCount = fz_count_pages(m_ctx, m_doc);
        m_path      = path;
        m_loaded    = true;
        m_needsPassword = false;
    }
    fz_catch(m_ctx) {
        emit loadError(QString("Errore: %1").arg(fz_caught_message(m_ctx)));
        return false;
    }
    return true;
}

void PdfDocument::close()
{
    QMutexLocker lock(&m_mutex);
    if (m_doc && m_ctx) { fz_drop_document(m_ctx, m_doc); m_doc = nullptr; }
    m_loaded = false; m_pageCount = 0; m_path.clear();
}

// ── pageInfo ──────────────────────────────────────────────────────────────────
PageInfo PdfDocument::pageInfo(int index) const
{
    PageInfo info;
    QMutexLocker lock(&m_mutex);
    if (!m_loaded || index < 0 || index >= m_pageCount) return info;
    fz_try(m_ctx) {
        fz_page *pg = fz_load_page(m_ctx, m_doc, index);
        fz_rect  r  = fz_bound_page(m_ctx, pg);
        info.sizePt = QSizeF(r.x1-r.x0, r.y1-r.y0);
        fz_drop_page(m_ctx, pg);
    }
    fz_catch(m_ctx) {}
    return info;
}

// ── renderPage ────────────────────────────────────────────────────────────────
QImage PdfDocument::renderPage(int index, float zoom, int rotation) const
{
    QMutexLocker lock(&m_mutex);
    if (!m_loaded || index < 0 || index >= m_pageCount) return {};

    QImage result;
    fz_try(m_ctx) {
        fz_page *pg = fz_load_page(m_ctx, m_doc, index);

        fz_matrix mat = fz_scale(zoom, zoom);
        if (rotation != 0) mat = fz_concat(mat, fz_rotate((float)rotation));

        fz_rect  bounds  = fz_transform_rect(fz_bound_page(m_ctx, pg), mat);
        fz_irect ibounds = fz_round_rect(bounds);
        int w = ibounds.x1-ibounds.x0, h = ibounds.y1-ibounds.y0;

        if (w > 0 && h > 0) {
            fz_pixmap *pix = fz_new_pixmap_with_bbox(
                m_ctx, fz_device_rgb(m_ctx), ibounds, nullptr, 1);
            fz_clear_pixmap_with_value(m_ctx, pix, 0xff);
            fz_device *dev = fz_new_draw_device(m_ctx, mat, pix);
            fz_run_page(m_ctx, pg, dev, fz_identity, nullptr);
            fz_close_device(m_ctx, dev);
            fz_drop_device(m_ctx, dev);

            int n      = fz_pixmap_components(m_ctx, pix);
            int stride = fz_pixmap_stride(m_ctx, pix);
            unsigned char *src = fz_pixmap_samples(m_ctx, pix);
            result = QImage(w, h, QImage::Format_RGB888);
            for (int y = 0; y < h; ++y) {
                unsigned char *dst = result.scanLine(y);
                unsigned char *row = src + y*stride;
                for (int x = 0; x < w; ++x) {
                    dst[x*3+0] = row[x*n+0];
                    dst[x*3+1] = row[x*n+1];
                    dst[x*3+2] = row[x*n+2];
                }
            }
            fz_drop_pixmap(m_ctx, pix);
        }
        fz_drop_page(m_ctx, pg);
    }
    fz_catch(m_ctx) {}
    return result;
}

QImage PdfDocument::renderThumbnail(int index, int widthPx) const
{
    PageInfo info = pageInfo(index);
    if (info.sizePt.width() <= 0) return {};
    return renderPage(index, (float)widthPx/(float)info.sizePt.width(), 0);
}

// ── extractText ───────────────────────────────────────────────────────────────
PageText PdfDocument::extractText(int pageIndex) const
{
    PageText result;
    QMutexLocker lock(&m_mutex);
    if (!m_loaded || pageIndex < 0 || pageIndex >= m_pageCount) return result;

    fz_try(m_ctx) {
        fz_page *pg = fz_load_page(m_ctx, m_doc, pageIndex);
        fz_stext_options opts = {};
        opts.flags = FZ_STEXT_PRESERVE_SPANS;
        fz_stext_page *stext = fz_new_stext_page_from_page(m_ctx, pg, &opts);

        for (fz_stext_block *block = stext->first_block; block; block = block->next) {
            if (block->type != FZ_STEXT_BLOCK_TEXT) continue;
            for (fz_stext_line *line = block->u.t.first_line; line; line = line->next) {
                for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
                    fz_rect r = fz_rect_from_quad(ch->quad);
                    TextChar tc;
                    tc.bbox = QRectF(r.x0, r.y0, r.x1-r.x0, r.y1-r.y0);
                    if (ch->c <= 32) {
                        tc.ch = " ";
                    } else {
                        char32_t cp = static_cast<char32_t>(ch->c);
                        tc.ch = QString::fromUcs4(&cp, 1);
                    }
                    result.append(tc);
                }
                if (!result.isEmpty()) {
                    TextChar nl; nl.bbox = result.last().bbox; nl.ch = "\n";
                    result.append(nl);
                }
            }
        }
        fz_drop_stext_page(m_ctx, stext);
        fz_drop_page(m_ctx, pg);
    }
    fz_catch(m_ctx) {}
    return result;
}

// ── search (sync) ─────────────────────────────────────────────────────────────
QList<SearchMatch> PdfDocument::search(const QString &query) const
{
    QList<SearchMatch> results;
    QMutexLocker lock(&m_mutex);
    if (!m_loaded || query.isEmpty()) return results;

    QByteArray utf8 = query.toUtf8();
    for (int i = 0; i < m_pageCount; ++i) {
        fz_try(m_ctx) {
            fz_page *pg = fz_load_page(m_ctx, m_doc, i);
            fz_quad quads[256];
            int found = MUPDF_SEARCH(m_ctx, pg, utf8.constData(), quads, 256);
            fz_drop_page(m_ctx, pg);
            if (found > 0) {
                SearchMatch m; m.pageIndex = i;
                for (int q = 0; q < found; ++q) {
                    fz_rect r = fz_rect_from_quad(quads[q]);
                    m.rects.append(QRectF(r.x0, r.y0, r.x1-r.x0, r.y1-r.y0));
                }
                results.append(m);
            }
        }
        fz_catch(m_ctx) {}
    }
    return results;
}

// ── search (async) ────────────────────────────────────────────────────────────
void PdfDocument::searchAsync(const QString &query)
{
    if (!m_loaded || query.isEmpty()) {
        emit searchFinished({});
        return;
    }
    m_searchCancelled = false;

    // Lancia in background — emette searchProgress ogni 10 pagine
    auto future = QtConcurrent::run([this, query]() {
        QList<SearchMatch> results;
        QByteArray utf8 = query.toUtf8();
        int total = m_pageCount;

        for (int i = 0; i < total; ++i) {
            if (m_searchCancelled) {
                emit searchCancelled();
                return;
            }
            {
                QMutexLocker lock(&m_mutex);
                if (!m_loaded) break;
                fz_try(m_ctx) {
                    fz_page *pg = fz_load_page(m_ctx, m_doc, i);
                    fz_quad quads[256];
                    int found = MUPDF_SEARCH(m_ctx, pg, utf8.constData(), quads, 256);
                    fz_drop_page(m_ctx, pg);
                    if (found > 0) {
                        SearchMatch m; m.pageIndex = i;
                        for (int q = 0; q < found; ++q) {
                            fz_rect r = fz_rect_from_quad(quads[q]);
                            m.rects.append(QRectF(r.x0, r.y0, r.x1-r.x0, r.y1-r.y0));
                        }
                        results.append(m);
                    }
                }
                fz_catch(m_ctx) {}
            }
            // Emetti progresso ogni 10 pagine o all'ultima
            if (i % 10 == 0 || i == total-1)
                emit searchProgress(i+1, total, results);
        }
        emit searchFinished(results);
    });
}

void PdfDocument::cancelSearch()
{
    m_searchCancelled = true;
}

// ── bookmarks ─────────────────────────────────────────────────────────────────
QList<BookmarkItem> PdfDocument::bookmarks() const
{
    QList<BookmarkItem> result;
    QMutexLocker lock(&m_mutex);
    if (!m_loaded) return result;
    fz_try(m_ctx) {
        fz_outline *outline = fz_load_outline(m_ctx, m_doc);
        if (outline) { buildBookmarks(outline, result, 0); fz_drop_outline(m_ctx, outline); }
    }
    fz_catch(m_ctx) {}
    return result;
}

void PdfDocument::buildBookmarks(fz_outline *node, QList<BookmarkItem> &out, int level) const
{
    for (fz_outline *n = node; n; n = n->next) {
        BookmarkItem item;
        item.title     = QString::fromUtf8(n->title ? n->title : "");
        item.pageIndex = MUPDF_OUTLINE_PAGE(n);
        item.level     = level;
        out.append(item);
        if (n->down) buildBookmarks(n->down, out, level+1);
    }
}

// ── metadata ──────────────────────────────────────────────────────────────────
QString PdfDocument::metaString(const char *key) const
{
    QMutexLocker lock(&m_mutex);
    if (!m_loaded) return {};
    char buf[512] = {};
    fz_try(m_ctx) { fz_lookup_metadata(m_ctx, m_doc, key, buf, sizeof(buf)); }
    fz_catch(m_ctx) {}
    return QString::fromUtf8(buf);
}
