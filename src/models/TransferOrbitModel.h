#ifndef TRANSFERORBITMODEL_H
#define TRANSFERORBITMODEL_H

#include <QAbstractListModel>
#include <QtQml/qqml.h>
#include <QVector3D>
#include <QColor>

struct OrbitSegment {
    QString name;
    QString displayName;
    QColor color;
    QList<QVector3D> points;
};

class TransferOrbitModel : public QAbstractListModel
{
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DisplayNameRole,
        ColorRole,
        PointsRole
    };

    explicit TransferOrbitModel(QObject *parent = nullptr);
    ~TransferOrbitModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadDefaultOrbit();
    Q_INVOKABLE bool loadFromCSV(const QString &filePath);
    Q_INVOKABLE void clear();
    
    // 设置控制点（从调试页面调用）
    Q_INVOKABLE void setControlPoints(const QList<QVector3D> &earthPoints,
                                       const QList<QVector3D> &transferPoints,
                                       const QList<QVector3D> &moonPoints);

signals:
    void segmentsChanged();

private:
    QList<OrbitSegment> m_segments;
};

#endif // TRANSFERORBITMODEL_H