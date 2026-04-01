#include "filesystemmodelcustom.h"

FileSystemModelCustom::FileSystemModelCustom(QObject *parent) : QFileSystemModel(parent) {
    // 空实现
}

QVariant FileSystemModelCustom::data( const QModelIndex& index, int role ) const {
    Q_UNUSED(index)
    Q_UNUSED(role)
    return QVariant();
}

Qt::ItemFlags FileSystemModelCustom::flags(const QModelIndex& index) const {
    Q_UNUSED(index)
    return Qt::NoItemFlags;
}