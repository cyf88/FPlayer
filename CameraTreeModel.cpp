#include "CameraTreeModel.h"

#include <QJsonArray>
#include <QJsonValue>

// ---------------- CameraTreeNode ----------------
CameraTreeNode::CameraTreeNode(NodeType type, QString id, QString label, CameraTreeNode *parent)
    : m_type(type), m_id(std::move(id)), m_label(std::move(label)), m_parent(parent)
{
}

CameraTreeNode::~CameraTreeNode()
{
    qDeleteAll(m_children);
    m_children.clear();
}

CameraTreeNode *CameraTreeNode::child(int row) const
{
    if (row < 0 || row >= m_children.size())
        return nullptr;
    return m_children.at(row);
}

int CameraTreeNode::row() const
{
    if (!m_parent)
        return 0;
    // 线性查找足够；后续如性能需要可缓存 index
    for (int i = 0; i < m_parent->m_children.size(); ++i) {
        if (m_parent->m_children.at(i) == this)
            return i;
    }
    return 0;
}

void CameraTreeNode::appendChild(CameraTreeNode *child)
{
    m_children.append(child);
}

// ---------------- CameraTreeModel ----------------
CameraTreeModel::CameraTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_root = new CameraTreeNode(CameraTreeNode::NodeType::Folder, QStringLiteral("root"), QStringLiteral("root"), nullptr);
}

CameraTreeModel::~CameraTreeModel()
{
    delete m_root;
    m_root = nullptr;
}

QModelIndex CameraTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0 || row < 0)
        return {};

    CameraTreeNode *parentNode = parent.isValid()
            ? static_cast<CameraTreeNode *>(parent.internalPointer())
            : m_root;

    CameraTreeNode *childNode = parentNode ? parentNode->child(row) : nullptr;
    if (!childNode)
        return {};

    return createIndex(row, column, childNode);
}

QModelIndex CameraTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};

    auto *childNode = static_cast<CameraTreeNode *>(child.internalPointer());
    CameraTreeNode *parentNode = childNode ? childNode->parent() : nullptr;

    if (!parentNode || parentNode == m_root)
        return {};

    return createIndex(parentNode->row(), 0, parentNode);
}

int CameraTreeModel::rowCount(const QModelIndex &parent) const
{
    CameraTreeNode *parentNode = parent.isValid()
            ? static_cast<CameraTreeNode *>(parent.internalPointer())
            : m_root;
    return parentNode ? parentNode->childCount() : 0;
}

int CameraTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant CameraTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    auto *node = static_cast<CameraTreeNode *>(index.internalPointer());
    if (!node)
        return {};

    switch (role) {
    case Qt::DisplayRole:
    case LabelRole:
        return node->label();
    case IdRole:
        return node->id();
    case TypeRole:
        return node->type() == CameraTreeNode::NodeType::Folder ? QStringLiteral("folder") : QStringLiteral("camera");
    case CameraCountRole:
        if (node->type() != CameraTreeNode::NodeType::Folder)
            return 0;
        return cameraCountUnder(node);
    default:
        return {};
    }
}

QHash<int, QByteArray> CameraTreeModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {LabelRole, "label"},
        {TypeRole, "type"},
        {CameraCountRole, "cameraCount"}
    };
}

void CameraTreeModel::clear()
{
    beginResetModel();
    delete m_root;
    m_root = new CameraTreeNode(CameraTreeNode::NodeType::Folder, QStringLiteral("root"), QStringLiteral("root"), nullptr);
    endResetModel();
}

void CameraTreeModel::setFromApiJson(const QJsonObject &rootObj)
{
    const auto dataVal = rootObj.value(QStringLiteral("data"));
    if (!dataVal.isArray()) {
        clear();
        return;
    }

    beginResetModel();
    delete m_root;
    m_root = new CameraTreeNode(CameraTreeNode::NodeType::Folder, QStringLiteral("root"), QStringLiteral("root"), nullptr);
    rebuildFromDataArray(dataVal.toArray());
    endResetModel();
}

void CameraTreeModel::rebuildFromDataArray(const QJsonArray &dataArray)
{
    for (const auto &v : dataArray) {
        if (!v.isObject())
            continue;
        buildFolderRecursive(m_root, v.toObject());
    }
}

void CameraTreeModel::buildFolderRecursive(CameraTreeNode *parent, const QJsonObject &folderObj)
{
    // folder: { id, label, children?, camera? }
    const auto idVal = folderObj.value(QStringLiteral("id"));
    const QString id = idVal.isString() ? idVal.toString() : QString::number(idVal.toVariant().toLongLong());
    const QString label = folderObj.value(QStringLiteral("label")).toString();

    auto *folderNode = new CameraTreeNode(CameraTreeNode::NodeType::Folder, id, label, parent);
    parent->appendChild(folderNode);

    // cameras under this folder
    const auto camVal = folderObj.value(QStringLiteral("camera"));
    if (camVal.isArray()) {
        const auto camArr = camVal.toArray();
        for (const auto &cv : camArr) {
            if (!cv.isObject())
                continue;
            const auto camObj = cv.toObject();
            const QString camId = camObj.value(QStringLiteral("id")).toString();
            const QString camName = camObj.value(QStringLiteral("name")).toString();
            auto *camNode = new CameraTreeNode(CameraTreeNode::NodeType::Camera, camId, camName, folderNode);
            folderNode->appendChild(camNode);
        }
    }

    // children folders
    const auto childrenVal = folderObj.value(QStringLiteral("children"));
    if (childrenVal.isArray()) {
        const auto childrenArr = childrenVal.toArray();
        for (const auto &childV : childrenArr) {
            if (!childV.isObject())
                continue;
            buildFolderRecursive(folderNode, childV.toObject());
        }
    }
}

QModelIndex CameraTreeModel::findFirstByLabel(const QString &text) const
{
    if (text.trimmed().isEmpty())
        return {};

    const auto needle = text.trimmed();
    return findFirstByLabelRecursive(m_root, needle);
}

QModelIndex CameraTreeModel::findFirstByLabelRecursive(CameraTreeNode *node, const QString &needle) const
{
    if (!node)
        return {};

    const auto label = node->label();
    if (node != m_root && label.contains(needle, Qt::CaseInsensitive)) {
        return createIndex(node->row(), 0, node);
    }

    for (int i = 0; i < node->childCount(); ++i) {
        auto *child = node->child(i);
        auto idx = findFirstByLabelRecursive(child, needle);
        if (idx.isValid())
            return idx;
    }
    return {};
}

QVariantList CameraTreeModel::findCamerasByLabel(const QString &text) const
{
    QVariantList out;
    const auto needle = text.trimmed();
    if (needle.isEmpty())
        return out;

    collectCamerasByLabelRecursive(m_root, needle, out);
    return out;
}

void CameraTreeModel::collectCamerasByLabelRecursive(CameraTreeNode *node, const QString &needle, QVariantList &out) const
{
    if (!node)
        return;

    if (node != m_root
        && node->type() == CameraTreeNode::NodeType::Camera
        && node->label().contains(needle, Qt::CaseInsensitive)) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), node->id());
        item.insert(QStringLiteral("label"), node->label());
        out.append(item);
    }

    for (int i = 0; i < node->childCount(); ++i)
        collectCamerasByLabelRecursive(node->child(i), needle, out);
}

int CameraTreeModel::cameraCountUnder(CameraTreeNode *node) const
{
    if (!node)
        return 0;

    int count = 0;
    for (int i = 0; i < node->childCount(); ++i) {
        auto *child = node->child(i);
        if (!child)
            continue;
        if (child->type() == CameraTreeNode::NodeType::Camera)
            count += 1;
        else
            count += cameraCountUnder(child);
    }
    return count;
}

