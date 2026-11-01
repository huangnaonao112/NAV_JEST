#include "UserModel.h"

#include "server/infra/Db.h"
#include "server/domain/user/UserRepository.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

UserModel::UserModel(QObject *parent)
    : QAbstractListModel(parent),
      m_nextRowNumber(1),
      m_db(new Db()),
      m_repo(nullptr) {
    m_db->open();
    m_repo = new UserRepository(m_db->database());
    m_repo->init();
}

UserModel::~UserModel() {
    delete m_repo;
    m_repo = nullptr;
    delete m_db;
    m_db = nullptr;
}

int UserModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_users.size();
}

QVariant UserModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return {};
    const int row = index.row();
    if (row < 0 || row >= m_users.size()) return {};

    const User &u = m_users[row];
    switch (role) {
    case RowNumberRole: return u.rowNumber;
    case UsernameRole: return u.username;
    case PasswordRole: return u.password;
    case PhoneRole: return u.phone;
    case DeptRole: return u.dept;
    case StatusRole: return u.status;
    case UpdatedAtRole: return u.updatedAt.toString("yyyy/MM/dd hh:mm:ss");
    default:
        return {};
    }
}

QHash<int, QByteArray> UserModel::roleNames() const {
    return {
        {RowNumberRole, "rowNumber"},
        {UsernameRole, "username"},
        {PasswordRole, "password"},
        {PhoneRole, "phone"},
        {DeptRole, "dept"},
        {StatusRole, "status"},
        {UpdatedAtRole, "updatedAt"}
    };
}

QVariantMap UserModel::get(int index) const {
    QVariantMap m;
    if (index < 0 || index >= m_users.size()) return m;
    const User &u = m_users[index];
    m["rowNumber"] = u.rowNumber;
    m["username"] = u.username;
    m["password"] = u.password;
    m["phone"] = u.phone;
    m["dept"] = u.dept;
    m["status"] = u.status;
    m["updatedAt"] = u.updatedAt.toString("yyyy/MM/dd hh:mm:ss");
    return m;
}

int UserModel::findByUsername(const QString &username) const {
    for (int i = 0; i < m_users.size(); ++i) {
        if (m_users[i].username.compare(username, Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

bool UserModel::add(const QString &username,
                    const QString &password,
                    const QString &phone,
                    const QString &dept,
                    const QString &status) {
    const QString normalized = username.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }
    if (findByUsername(normalized) != -1) {
        return false;
    }

    User u;
    u.rowNumber = m_nextRowNumber++;
    u.username = normalized;
    u.password = password;
    u.phone = phone.trimmed();
    u.dept = dept.trimmed();
    u.status = status.trimmed();
    u.updatedAt = QDateTime::currentDateTime();

    if (m_repo) {
        UserRepository::User ru;
        ru.userId = u.username;         // UserModel.username -> USER_ID (主键)
        ru.userName = u.username;       // UserModel.username -> USER_NAME (显示名称)
        ru.userPassword = u.password;   // UserModel.password -> USER_PASSWORD
        ru.userLevel = 2;               // 默认操作员级别
        ru.phone = u.phone;
        ru.dept = u.dept;
        ru.status = u.status;
        ru.updatedAt = u.updatedAt;
        if (!m_repo->insert(ru)) {
            return false;
        }
    }

    beginInsertRows(QModelIndex(), m_users.size(), m_users.size());
    m_users.push_back(u);
    endInsertRows();

    return true;
}

bool UserModel::update(int index,
                       const QString &username,
                       const QString &password,
                       const QString &phone,
                       const QString &dept,
                       const QString &status) {
    if (index < 0 || index >= m_users.size()) return false;
    const QString normalized = username.trimmed();
    if (normalized.isEmpty()) return false;

    const int existing = findByUsername(normalized);
    if (existing != -1 && existing != index) return false;

    User &u = m_users[index];
    const QString oldUsername = u.username;

    u.username = normalized;
    if (!password.isEmpty()) {
        u.password = password;
    }
    u.phone = phone.trimmed();
    u.dept = dept.trimmed();
    u.status = status.trimmed();
    u.updatedAt = QDateTime::currentDateTime();

    if (m_repo) {
        UserRepository::User ru;
        ru.userId = u.username;         // UserModel.username -> User_ID (主键)
        ru.userName = u.username;       // UserModel.username -> User_Name
        ru.userPassword = u.password;   // UserModel.password -> User_Password
        ru.userLevel = 2;               // 默认操作员级别
        ru.phone = u.phone;
        ru.dept = u.dept;
        ru.status = u.status;
        ru.updatedAt = u.updatedAt;
        if (!m_repo->update(oldUsername, ru)) {
            return false;
        }
    }

    const QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {UsernameRole, PasswordRole, PhoneRole, DeptRole, StatusRole, UpdatedAtRole, RowNumberRole});

    return true;
}

bool UserModel::removeAt(int index) {
    if (index < 0 || index >= m_users.size()) return false;
    const QString username = m_users[index].username;

    if (m_repo) {
        if (!m_repo->remove(username)) {
            return false;
        }
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_users.removeAt(index);
    endRemoveRows();

    // reindex rowNumber to keep UI consistent
    for (int i = 0; i < m_users.size(); ++i) {
        m_users[i].rowNumber = i + 1;
    }
    m_nextRowNumber = m_users.size() + 1;

    // sync db row_number
    if (m_repo) {
        for (const auto &u : m_users) {
            UserRepository::User ru;
            ru.userId = u.username;
            ru.userName = u.username;
            ru.userPassword = u.password;
            ru.userLevel = 2;
            ru.phone = u.phone;
            ru.dept = u.dept;
            ru.status = u.status;
            ru.updatedAt = u.updatedAt;
            m_repo->update(u.username, ru);
        }
    }

    return true;
}

bool UserModel::load() {
    if (!m_repo) return false;

    auto rows = m_repo->list();
    if (rows.isEmpty()) {
        // seed default users similar to original mock data
        const QDateTime now = QDateTime::currentDateTime();

        auto ensure = [&](int /*rowNumber*/,
                          const QString &username,
                          const QString &password,
                          const QString &phone,
                          const QString &dept,
                          const QString &status) {
            if (m_repo->exists(username)) return;
            UserRepository::User ru;
            ru.userId = username;
            ru.userName = username;
            ru.userPassword = password;
            ru.userLevel = 2;
            ru.phone = phone;
            ru.dept = dept;
            ru.status = status;
            ru.updatedAt = now;
            m_repo->insert(ru);
        };

        ensure(1, QStringLiteral("admin"), QStringLiteral("123456"), QStringLiteral("189XXXXXXXX"), QStringLiteral("研发部"), QStringLiteral("正常"));
        ensure(2, QStringLiteral("test1"), QStringLiteral("123456"), QStringLiteral("189XXXXXXXX"), QStringLiteral("研发部"), QStringLiteral("正常"));
        ensure(3, QStringLiteral("test2"), QStringLiteral("123456"), QStringLiteral("189XXXXXXXX"), QStringLiteral("研发部"), QStringLiteral("正常"));

        rows = m_repo->list();
    }

    beginResetModel();
    m_users.clear();
    m_users.reserve(rows.size());
    m_nextRowNumber = 1;

    for (const auto &ru : rows) {
        User u;
        u.rowNumber = m_nextRowNumber++;  // 本地行号
        u.username = ru.userId;           // User_ID 作为 username
        u.password = ru.userPassword;
        u.phone = ru.phone;               // 读取 phone 字段
        u.dept = ru.dept;                 // 读取 dept 字段
        u.status = ru.status.isEmpty() ? QStringLiteral("正常") : ru.status;  // 读取 status 字段
        u.updatedAt = ru.updatedAt;
        if (u.username.trimmed().isEmpty()) continue;
        m_users.push_back(u);
    }

    endResetModel();
    return true;
}

bool UserModel::save() const {
    return true;
}

QString UserModel::storagePath() const {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(base);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("earthserver.sqlite");
}
