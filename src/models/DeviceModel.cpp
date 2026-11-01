#include "DeviceModel.h"

#include "server/infra/Db.h"
#include "server/domain/device/DeviceRepository.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

DeviceModel::DeviceModel(QObject *parent)
    : QAbstractListModel(parent),
      m_db(new Db()),
      m_repo(nullptr) {
    m_db->open();
    m_sql = m_db->database();
    m_repo = new DeviceRepository(m_sql);
    m_repo->init();
}

DeviceModel::~DeviceModel() {
    delete m_repo;
    m_repo = nullptr;
    delete m_db;
    m_db = nullptr;
}

static QString legacyDevicesJsonPath() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(base);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("devices.json");
}

static bool importLegacyDevicesJson(DeviceRepository &repo, const QString &path) {
    QFile f(path);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return false;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return false;

    const QJsonArray arr = doc.array();
    bool importedAny = false;
    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();

        DeviceRepository::Device d;
        d.id = o.value("deviceId").toString().trimmed();
        d.name = o.value("name").toString().trimmed();
        d.type = o.value("type").toString().trimmed();
        d.model = o.value("model").toString().trimmed();
        d.vendor = o.value("vendor").toString().trimmed();
        d.connection = o.value("connection").toString().trimmed();
        d.address = o.value("address").toString().trimmed();
        d.updatedAt = QDateTime::currentDateTime();

        if (d.id.isEmpty()) continue;
        if (repo.exists(d.id)) continue;
        if (repo.insert(d)) {
            importedAny = true;
        }
    }
    return importedAny;
}

static void seedDefaultDevices(DeviceRepository &repo) {
    const QDateTime now = QDateTime::currentDateTime();

    auto ensure = [&](const QString &id,
                      const QString &name,
                      const QString &type,
                      const QString &model,
                      const QString &vendor,
                      const QString &connection,
                      const QString &address) {
        if (repo.exists(id)) return;
        DeviceRepository::Device d;
        d.id = id;
        d.name = name;
        d.type = type;
        d.model = model;
        d.vendor = vendor;
        d.connection = connection;
        d.address = address;
        d.updatedAt = now;
        repo.insert(d);
    };

    ensure(QStringLiteral("CETC001"), QStringLiteral("待检设备"), QStringLiteral("待检设备"), QStringLiteral("CETC001"), QStringLiteral("CETC"), QStringLiteral("TCP/IP"), QStringLiteral("127.0.0.1"));
    ensure(QStringLiteral("CETC002"), QStringLiteral("干扰模拟源"), QStringLiteral("模拟源"), QStringLiteral("CETC002"), QStringLiteral("CETC"), QStringLiteral("TCP/IP"), QStringLiteral("127.0.0.1"));
    ensure(QStringLiteral("CETC003"), QStringLiteral("压制干扰源"), QStringLiteral("模拟源"), QStringLiteral("CETC003"), QStringLiteral("CETC"), QStringLiteral("TCP/IP"), QStringLiteral("127.0.0.1"));
    ensure(QStringLiteral("CETC004"), QStringLiteral("无人机载终端"), QStringLiteral("待检设备"), QStringLiteral("CETC004"), QStringLiteral("CETC"), QStringLiteral("TCP/IP"), QStringLiteral("127.0.0.1"));
}

int DeviceModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_devices.size();
}

QVariant DeviceModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return {};
    const int row = index.row();
    if (row < 0 || row >= m_devices.size()) return {};

    const Device &d = m_devices[row];
    switch (role) {
    case DeviceIdRole: return d.deviceId;
    case NameRole: return d.name;
    case TypeRole: return d.type;
    case ModelRole: return d.model;
    case VendorRole: return d.vendor;
    case ConnectionRole: return d.connection;
    case AddressRole: return d.address;
    case UpdatedAtRole: return d.updatedAt.toString("yyyy/MM/dd hh:mm:ss");
    default:
        return {};
    }
}

QHash<int, QByteArray> DeviceModel::roleNames() const {
    return {
        {DeviceIdRole, "deviceId"},
        {NameRole, "name"},
        {TypeRole, "type"},
        {ModelRole, "model"},
        {VendorRole, "vendor"},
        {ConnectionRole, "connection"},
        {AddressRole, "address"},
        {UpdatedAtRole, "updatedAt"}
    };
}

QVariantMap DeviceModel::get(int index) const {
    QVariantMap m;
    if (index < 0 || index >= m_devices.size()) return m;
    const Device &d = m_devices[index];
    m["deviceId"] = d.deviceId;
    m["name"] = d.name;
    m["type"] = d.type;
    m["model"] = d.model;
    m["vendor"] = d.vendor;
    m["connection"] = d.connection;
    m["address"] = d.address;
    m["updatedAt"] = d.updatedAt.toString("yyyy/MM/dd hh:mm:ss");
    return m;
}

int DeviceModel::findByDeviceId(const QString &deviceId) const {
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].deviceId.compare(deviceId, Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

bool DeviceModel::add(const QString &deviceId,
                      const QString &name,
                      const QString &type,
                      const QString &model,
                      const QString &vendor,
                      const QString &connection,
                      const QString &address) {
    if (deviceId.trimmed().isEmpty()) return false;
    if (findByDeviceId(deviceId) != -1) return false;

    Device d;
    d.deviceId = deviceId.trimmed();
    d.name = name.trimmed();
    d.type = type.trimmed();
    d.model = model.trimmed();
    d.vendor = vendor.trimmed();
    d.connection = connection.trimmed();
    d.address = address.trimmed();
    d.updatedAt = QDateTime::currentDateTime();

    if (m_repo) {
        DeviceRepository::Device rd;
        rd.id = d.deviceId;
        rd.name = d.name;
        rd.type = d.type;
        rd.model = d.model;
        rd.vendor = d.vendor;
        rd.connection = d.connection;
        rd.address = d.address;
        rd.updatedAt = d.updatedAt;
        if (!m_repo->insert(rd)) {
            return false;
        }
    }

    beginInsertRows(QModelIndex(), m_devices.size(), m_devices.size());
    m_devices.push_back(d);
    endInsertRows();

    return true;
}

bool DeviceModel::update(int index,
                         const QString &deviceId,
                         const QString &name,
                         const QString &type,
                         const QString &model,
                         const QString &vendor,
                         const QString &connection,
                         const QString &address) {
    if (index < 0 || index >= m_devices.size()) return false;
    const QString normalizedId = deviceId.trimmed();
    if (normalizedId.isEmpty()) return false;

    const int existing = findByDeviceId(normalizedId);
    if (existing != -1 && existing != index) return false;

    Device &d = m_devices[index];
    const QString oldId = d.deviceId;
    d.deviceId = normalizedId;
    d.name = name.trimmed();
    d.type = type.trimmed();
    d.model = model.trimmed();
    d.vendor = vendor.trimmed();
    d.connection = connection.trimmed();
    d.address = address.trimmed();
    d.updatedAt = QDateTime::currentDateTime();

    if (m_repo) {
        DeviceRepository::Device rd;
        rd.id = d.deviceId;
        rd.name = d.name;
        rd.type = d.type;
        rd.model = d.model;
        rd.vendor = d.vendor;
        rd.connection = d.connection;
        rd.address = d.address;
        rd.updatedAt = d.updatedAt;
        if (!m_repo->update(oldId, rd)) {
            return false;
        }
    }

    const QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {DeviceIdRole, NameRole, TypeRole, ModelRole, VendorRole, ConnectionRole, AddressRole, UpdatedAtRole});

    return true;
}

bool DeviceModel::removeAt(int index) {
    if (index < 0 || index >= m_devices.size()) return false;

    const QString id = m_devices[index].deviceId;
    if (m_repo) {
        if (!m_repo->remove(id)) {
            return false;
        }
    }
    beginRemoveRows(QModelIndex(), index, index);
    m_devices.removeAt(index);
    endRemoveRows();
    return true;
}

QString DeviceModel::storagePath() const {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(base);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("earthserver.sqlite");
}

QString DeviceModel::isoString(const QDateTime &dt) {
    if (!dt.isValid()) return {};
    return dt.toString(Qt::ISODateWithMs);
}

QDateTime DeviceModel::fromIsoString(const QString &s) {
    if (s.trimmed().isEmpty()) return {};
    QDateTime dt = QDateTime::fromString(s, Qt::ISODateWithMs);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(s, Qt::ISODate);
    }
    return dt;
}

bool DeviceModel::load() {
    if (!m_repo) return false;

    auto rows = m_repo->list();
    if (rows.isEmpty()) {
        const QString legacyPath = legacyDevicesJsonPath();
        if (!importLegacyDevicesJson(*m_repo, legacyPath)) {
            seedDefaultDevices(*m_repo);
        }
        rows = m_repo->list();
    }
    beginResetModel();
    m_devices.clear();
    m_devices.reserve(rows.size());
    for (const auto &rd : rows) {
        Device d;
        d.deviceId = rd.id;
        d.name = rd.name;
        d.type = rd.type;
        d.model = rd.model;
        d.vendor = rd.vendor;
        d.connection = rd.connection;
        d.address = rd.address;
        d.updatedAt = rd.updatedAt;
        if (d.deviceId.trimmed().isEmpty()) continue;
        m_devices.push_back(d);
    }
    endResetModel();
    return true;
}

bool DeviceModel::save() const {
    return true;
}
