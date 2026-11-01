#pragma once

#include <QAbstractListModel>
#include <QtQml/qqml.h>
#include <QDateTime>
#include <QString>
#include <QVector>
#include <QVariantMap>

class Db;
class UserRepository;

class UserModel : public QAbstractListModel {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
public:
    enum Roles {
        RowNumberRole = Qt::UserRole + 1,
        UsernameRole,
        PasswordRole,
        PhoneRole,
        DeptRole,
        StatusRole,
        UpdatedAtRole
    };

    explicit UserModel(QObject *parent = nullptr);
    ~UserModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;

    Q_INVOKABLE bool add(const QString &username,
                         const QString &password,
                         const QString &phone,
                         const QString &dept,
                         const QString &status);

    Q_INVOKABLE bool update(int index,
                            const QString &username,
                            const QString &password,
                            const QString &phone,
                            const QString &dept,
                            const QString &status);

    Q_INVOKABLE bool removeAt(int index);

    Q_INVOKABLE bool load();
    Q_INVOKABLE bool save() const;

    Q_INVOKABLE QString storagePath() const;

private:
    struct User {
        int rowNumber;
        QString username;
        QString password;
        QString phone;
        QString dept;
        QString status;
        QDateTime updatedAt;
    };

    int findByUsername(const QString &username) const;

    QVector<User> m_users;
    int m_nextRowNumber;

    Db *m_db;
    UserRepository *m_repo;
};
