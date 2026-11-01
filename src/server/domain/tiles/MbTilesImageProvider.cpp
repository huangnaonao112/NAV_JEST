#include "MbTilesImageProvider.h"

#include "MbTiles.h"

#include <QImage>
#include <QPainter>
#include <QDir>
#include <QDebug>

MbTilesImageProvider::MbTilesImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {
}

MbTilesImageProvider::~MbTilesImageProvider() = default;

void MbTilesImageProvider::addMbTiles(const QString &name, const QString &path,
                                       int minZoom, int maxZoom,
                                       double boundsWest, double boundsSouth,
                                       double boundsEast, double boundsNorth) {
    TileSource src;
    src.mbtiles.reset(new MbTiles(path));
    if (!src.mbtiles->open()) {
        qWarning() << "MbTilesImageProvider: Failed to open" << name << "at" << path;
        return;
    }
    src.minZoom = minZoom;
    src.maxZoom = maxZoom;
    src.boundsWest = boundsWest;
    src.boundsSouth = boundsSouth;
    src.boundsEast = boundsEast;
    src.boundsNorth = boundsNorth;
    m_sources.append(std::move(src));
}

namespace {
constexpr int kTileSize = 256;

// 瓦片坐标转经纬度 (WebMercator)
void tileToLonLat(int x, int y, int z, double &lon, double &lat) {
    const int n = 1 << z;
    lon = x * 360.0 / n - 180.0;
    const double latRad = atan(sinh(M_PI * (1 - 2.0 * y / n)));
    lat = latRad * 180.0 / M_PI;
}

static QImage placeholderTile(const QString &label) {
    QImage img(kTileSize, kTileSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(26, 26, 46));
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QColor(90, 90, 120));
    p.drawRect(0, 0, kTileSize - 1, kTileSize - 1);
    p.drawLine(0, 0, kTileSize - 1, kTileSize - 1);
    p.drawLine(kTileSize - 1, 0, 0, kTileSize - 1);
    p.setPen(QColor(200, 200, 220));
    p.drawText(QRect(8, 8, kTileSize - 16, kTileSize - 16), Qt::AlignLeft | Qt::AlignTop, label);
    return img;
}

static bool parseId(const QString &id, int &z, int &x, int &y) {
    const QStringList parts = id.split('/', Qt::SkipEmptyParts);
    if (parts.size() < 3) return false;
    bool okZ=false, okX=false, okY=false;
    z = parts[0].toInt(&okZ);
    x = parts[1].toInt(&okX);
    QString yPart = parts[2];
    const int dot = yPart.indexOf('.');
    if (dot >= 0) yPart = yPart.left(dot);
    y = yPart.toInt(&okY);
    return okZ && okX && okY;
}

static QImage cropFallback(const QImage &parent, int zDiff, int x, int y) {
    // parent is at z-(zDiff), returns a 256x256 crop for child (x,y) at target z.
    const int factor = 1 << zDiff;
    const int cx = x & (factor - 1);
    const int cy = y & (factor - 1);

    const int srcSize = kTileSize / factor;
    if (srcSize <= 0) return QImage();

    const QRect srcRect(cx * srcSize, cy * srcSize, srcSize, srcSize);
    QImage cropped = parent.copy(srcRect);
    return cropped.scaled(kTileSize, kTileSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}
} // namespace

MbTilesImageProvider::TileSource* MbTilesImageProvider::selectSource(int z, int x, int y) {
    // 计算瓦片中心经纬度
    double lon, lat;
    tileToLonLat(x, y, z, lon, lat);
    
    // 优先选择zoom级别匹配且在范围内的数据源
    for (auto &src : m_sources) {
        if (z >= src.minZoom && z <= src.maxZoom) {
            // 检查是否在边界内
            if (lon >= src.boundsWest && lon <= src.boundsEast &&
                lat >= src.boundsSouth && lat <= src.boundsNorth) {
                return &src;
            }
        }
    }
    
    // 如果没有精确匹配，选择zoom范围匹配的数据源（忽略边界）
    for (auto &src : m_sources) {
        if (z >= src.minZoom && z <= src.maxZoom) {
            return &src;
        }
    }
    
    // 尝试从高zoom数据源获取（石家庄fallback到全国）
    for (auto &src : m_sources) {
        if (z >= src.minZoom - 5 && z <= src.maxZoom + 5) {
            return &src;
        }
    }
    
    return nullptr;
}

QImage MbTilesImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    Q_UNUSED(requestedSize)

    int z=0, x=0, y=0;
    if (!parseId(id, z, x, y)) {
        return placeholderTile(QStringLiteral("bad id\n%1").arg(id));
    }

    TileSource *src = selectSource(z, x, y);
    if (!src) {
        return QImage(); // 无数据源
    }

    auto loadAt = [&](MbTiles *tiles, int zz, int xx, int yy, QImage &out) -> bool {
        QByteArray data;
        QString mime;
        if (!tiles->getTile(zz, xx, yy, data, mime)) return false;
        QImage img;
        if (!img.loadFromData(data)) return false;
        out = img;
        return true;
    };

    QImage img;
    if (!loadAt(src->mbtiles.data(), z, x, y, img)) {
        // 尝试从父级瓦片裁剪
        const int minZ = src->mbtiles->minZoom();
        for (int zz = z - 1; zz >= minZ; --zz) {
            const int diff = z - zz;
            if (diff > 8) break;
            const int px = x >> diff;
            const int py = y >> diff;
            QImage parent;
            if (!loadAt(src->mbtiles.data(), zz, px, py, parent)) continue;
            QImage cropped = cropFallback(parent, diff, x, y);
            if (!cropped.isNull()) {
                img = std::move(cropped);
                break;
            }
        }
    }

    if (img.isNull()) {
        // Outside coverage: let the background show instead of drawing debug tiles.
        return QImage();
    }

    if (size) {
        *size = img.size();
    }
    return img;
}
