#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>

class MbTiles {
public:
    explicit MbTiles(QString mbtilesPath);

    bool open();

    QString path() const { return m_path; }
    int minZoom() const { return m_minZoom; }
    int maxZoom() const { return m_maxZoom; }

    // Strict check (hits SQLite). Fast-reject using cached ranges first.
    bool hasTile(int z, int x, int y);

    // Returns true if tile exists. Output mimeType: "image/png" or "image/jpeg".
    bool getTile(int z, int x, int y, QByteArray &outData, QString &outMimeType);

private:
    struct ZoomRange {
        int minX = 0;
        int maxX = -1;
        int minTmsY = 0;
        int maxTmsY = -1;
        bool valid() const { return maxX >= minX && maxTmsY >= minTmsY; }
    };

    bool ensureMetaLoaded();
    bool fastMaybeHasTile(int z, int x, int y) const;

    QString m_path;
    bool m_metaLoaded = false;
    int m_minZoom = 0;
    int m_maxZoom = 0;
    QHash<int, ZoomRange> m_rangesByZoom;
};
