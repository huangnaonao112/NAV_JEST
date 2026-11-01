#pragma once

#include <QQuickImageProvider>
#include <QSharedPointer>

class MbTiles;

/**
 * @brief 多数据源瓦片图像提供器
 * 支持：
 * - 全国10级 (03091035.mbtiles)
 * - 石家庄15级 (石家庄市高德卫星03170927.mbtiles)
 */
class MbTilesImageProvider : public QQuickImageProvider {
public:
    explicit MbTilesImageProvider();
    ~MbTilesImageProvider();

    // 添加数据源
    void addMbTiles(const QString &name, const QString &path, int minZoom, int maxZoom,
                    double boundsWest, double boundsSouth, double boundsEast, double boundsNorth);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    struct TileSource {
        QSharedPointer<MbTiles> mbtiles;
        int minZoom;
        int maxZoom;
        double boundsWest, boundsSouth, boundsEast, boundsNorth;
    };
    QList<TileSource> m_sources;

    // 根据zoom和坐标选择合适的数据源
    TileSource* selectSource(int z, int x, int y);
};