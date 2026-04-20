#pragma once

#include <QAbstractItemModel>
#include <QJsonObject>
#include <QVariantList>
#include <QVector>

class CameraTreeNode
{
public:
    enum class NodeType { Folder, Camera };

    CameraTreeNode(NodeType type,
                   QString id,
                   QString label,
                   CameraTreeNode *parent = nullptr);
    ~CameraTreeNode();

    CameraTreeNode *parent() const { return m_parent; }
    CameraTreeNode *child(int row) const;
    int childCount() const { return m_children.size(); }
    int row() const;
    void appendChild(CameraTreeNode *child);

    NodeType type() const { return m_type; }
    const QString &id() const { return m_id; }
    const QString &label() const { return m_label; }

private:
    NodeType m_type;
    QString m_id;
    QString m_label;
    CameraTreeNode *m_parent = nullptr;
    QVector<CameraTreeNode *> m_children;
};

class CameraTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        LabelRole,
        TypeRole,
        CameraCountRole
    };

    explicit CameraTreeModel(QObject *parent = nullptr);
    ~CameraTreeModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();
    void setFromApiJson(const QJsonObject &rootObj);
    Q_INVOKABLE QModelIndex findFirstByLabel(const QString &text) const;
    Q_INVOKABLE QVariantList findCamerasByLabel(const QString &text) const;

private:
    CameraTreeNode *m_root = nullptr;

    void rebuildFromDataArray(const QJsonArray &dataArray);
    void buildFolderRecursive(CameraTreeNode *parent, const QJsonObject &folderObj);

    QModelIndex findFirstByLabelRecursive(CameraTreeNode *node, const QString &needle) const;
    void collectCamerasByLabelRecursive(CameraTreeNode *node, const QString &needle, QVariantList &out) const;

    int cameraCountUnder(CameraTreeNode *node) const;
};

