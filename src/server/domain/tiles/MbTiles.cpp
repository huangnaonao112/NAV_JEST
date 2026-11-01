#include "MbTiles.h"

#include <QFile>
#include <QDebug>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QThread>
#include <QVariant>

static QString detectMime(const QByteArray &data) {
    if (data.size() >= 8 && (uchar)data[0] == 0x89 && data.mid(1, 3) == "PNG") {
        return "image/png";
    }
    if (data.size() >= 3 && (uchar)data[0] == 0xFF && (uchar)data[1] == 0xD8 && (uchar)data[2] == 0xFF) {
        return "image/jpeg";
    }
    return "application/octet-stream";
}

MbTiles::MbTiles(QString mbtilesPath)
    : m_path(std::move(mbtilesPath)) {
}

static QSqlDatabase dbForThread(const QString &path) {
    const auto tid = reinterpret_cast<quintptr>(QThread::currentThreadId());
    const QString connName = QStringLiteral("mbtiles_%1_%2").arg(qHash(path)).arg(tid);
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) return db;
        if (db.open()) return db;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(path);
    db.open();
    return db;
}

bool MbTiles::open() {
    if (!QFile::exists(m_path)) {
        qWarning() << "MbTiles: file not found:" << m_path;
        return false;
    }

    // Only validates file and loads metadata; actual tile reads open per-thread connections.
    QSqlDatabase db = dbForThread(m_path);
    if (!db.isOpen()) {
        qWarning() << "MbTiles: failed to open database:" << m_path << db.lastError();
        return false;
    }

    return ensureMetaLoaded();
}

bool MbTiles::ensureMetaLoaded() {
    if (m_metaLoaded) return true;

    QSqlDatabase db = dbForThread(m_path);
    if (!db.isOpen()) return false;

    // Prefer metadata table.
    int minz = -1;
    int maxz = -1;
    {
        QSqlQuery q(db);
        q.prepare("SELECT value FROM metadata WHERE name=? LIMIT 1");
        q.addBindValue(QStringLiteral("minzoom"));
        if (q.exec() && q.next()) minz = q.value(0).toInt();
    }
    {
        QSqlQuery q(db);
        q.prepare("SELECT value FROM metadata WHERE name=? LIMIT 1");
        q.addBindValue(QStringLiteral("maxzoom"));
        if (q.exec() && q.next()) maxz = q.value(0).toInt();
    }
    if (minz < 0 || maxz < 0) {
        QSqlQuery q(db);
        if (q.exec("SELECT MIN(zoom_level), MAX(zoom_level) FROM tiles") && q.next()) {
            minz = q.value(0).toInt();
            maxz = q.value(1).toInt();
        }
    }
    if (minz < 0 || maxz < 0) {
        qWarning() << "MbTiles: failed to determine min/max zoom:" << m_path;
        return false;
    }

    m_minZoom = minz;
    m_maxZoom = maxz;

    // Cache coarse ranges per zoom (TMS y-range).
    m_rangesByZoom.clear();
    QSqlQuery q(db);
    if (q.exec("SELECT zoom_level, MIN(tile_column), MAX(tile_column), MIN(tile_row), MAX(tile_row) FROM tiles GROUP BY zoom_level")) {
        while (q.next()) {
            const int z = q.value(0).toInt();
            ZoomRange r;
            r.minX = q.value(1).toInt();
            r.maxX = q.value(2).toInt();
            r.minTmsY = q.value(3).toInt();
            r.maxTmsY = q.value(4).toInt();
            m_rangesByZoom.insert(z, r);
        }
    }

    m_metaLoaded = true;
    return true;
}

bool MbTiles::fastMaybeHasTile(int z, int x, int y) const {
    if (!m_metaLoaded) return true;
    if (z < m_minZoom || z > m_maxZoom) return false;

    const auto it = m_rangesByZoom.constFind(z);
    if (it == m_rangesByZoom.constEnd()) return false;
    const ZoomRange &r = it.value();
    if (!r.valid()) return false;

    const int n = 1 << z;
    const int tmsY = (n - 1) - y;
    return x >= r.minX && x <= r.maxX && tmsY >= r.minTmsY && tmsY <= r.maxTmsY;
}

bool MbTiles::hasTile(int z, int x, int y) {
    if (!ensureMetaLoaded()) return false;
    if (!fastMaybeHasTile(z, x, y)) return false;

    QSqlDatabase db = dbForThread(m_path);
    if (!db.isOpen()) return false;

    const int n = 1 << z;
    const int tmsY = (n - 1) - y;

    QSqlQuery q(db);
    q.prepare("SELECT 1 FROM tiles WHERE zoom_level=? AND tile_column=? AND tile_row=? LIMIT 1");
    q.addBindValue(z);
    q.addBindValue(x);
    q.addBindValue(tmsY);
    if (!q.exec()) return false;
    return q.next();
}

bool MbTiles::getTile(int z, int x, int y, QByteArray &outData, QString &outMimeType) {
    if (!ensureMetaLoaded()) {
        if (!open()) return false;
    }
    if (!fastMaybeHasTile(z, x, y)) return false;

    // XYZ -> TMS
    const int n = 1 << z;
    const int tmsY = (n - 1) - y;

    QSqlDatabase db = dbForThread(m_path);
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("SELECT tile_data FROM tiles WHERE zoom_level=? AND tile_column=? AND tile_row=? LIMIT 1");
    q.addBindValue(z);
    q.addBindValue(x);
    q.addBindValue(tmsY);

    if (!q.exec()) {
        qWarning() << "MbTiles::getTile: query failed for z=" << z << "x=" << x << "y=" << y << "tmsY=" << tmsY << q.lastError();
        return false;
    }
    if (!q.next()) {
        return false;
    }

    outData = q.value(0).toByteArray();
    outMimeType = detectMime(outData);
    if (outData.isEmpty()) {
        qWarning() << "MbTiles::getTile: empty tile data for z=" << z << "x=" << x << "y=" << y;
        return false;
    }
    return true;
}
