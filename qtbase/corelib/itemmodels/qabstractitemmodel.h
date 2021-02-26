#ifndef QABSTRACTITEMMODEL_H
#define QABSTRACTITEMMODEL_H


class QAbstractItemModel;
class QPersistentModelIndex;

class  QModelIndex
{
    friend class QAbstractItemModel;
public:
     inline QModelIndex() Q_DECL_NOTHROW : r(-1), c(-1), i(0), m(Q_NULLPTR) {}
    // compiler-generated copy/move ctors/assignment operators are fine!
     inline int row() const Q_DECL_NOTHROW { return r; }
     inline int column() const Q_DECL_NOTHROW { return c; }
     inline quintptr internalId() const Q_DECL_NOTHROW { return i; }
    inline void *internalPointer() const Q_DECL_NOTHROW { return reinterpret_cast<void*>(i); }
    inline QModelIndex parent() const{ return m ? m->parent(*this) : QModelIndex(); }
    inline QModelIndex sibling(int row, int column) const
		{ return m ? (r == arow && c == acolumn) ? *this : m->sibling(arow, acolumn, *this) : QModelIndex(); }


    inline QVariant data(int role = Qt::DisplayRole) const{ return m ? m->data(*this, arole) : QVariant(); }
    inline Qt::ItemFlags flags() const{ return m ? m->flags(*this) : Qt::ItemFlags(); }
     inline const QAbstractItemModel *model() const Q_DECL_NOTHROW { return m; }
     inline bool isValid() const Q_DECL_NOTHROW { return (r >= 0) && (c >= 0) && (m != Q_NULLPTR); }
     inline bool operator==(const QModelIndex &other) const Q_DECL_NOTHROW
        { return (other.r == r) && (other.i == i) && (other.c == c) && (other.m == m); }
     inline bool operator!=(const QModelIndex &other) const Q_DECL_NOTHROW
        { return !(*this == other); }
     inline bool operator<(const QModelIndex &other) const Q_DECL_NOTHROW
        {
            return  r <  other.r
                || (r == other.r && (c <  other.c
                                 || (c == other.c && (i <  other.i
                                                  || (i == other.i && std::less<const QAbstractItemModel *>()(m, other.m))))));
        }
private:
    inline QModelIndex(int arow, int acolumn, void *ptr, const QAbstractItemModel *amodel) 
        : r(arow), c(acolumn), i(reinterpret_cast<quintptr>(ptr)), m(amodel) {}
    inline QModelIndex(int arow, int acolumn, quintptr id, const QAbstractItemModel *amodel) 
        : r(arow), c(acolumn), i(id), m(amodel) {}
    int r, c;
    quintptr i;
    const QAbstractItemModel *m;
};


class QPersistentModelIndexData;

// qHash is a friend, but we can't use default arguments for friends (§8.3.6.4)
uint qHash(const QPersistentModelIndex &index, uint seed = 0) ;

class  QPersistentModelIndex
{
public:
    QPersistentModelIndex();
    QPersistentModelIndex(const QModelIndex &index);
    QPersistentModelIndex(const QPersistentModelIndex &other);
    ~QPersistentModelIndex();
    bool operator<(const QPersistentModelIndex &other) const;
    bool operator==(const QPersistentModelIndex &other) const;
    inline bool operator!=(const QPersistentModelIndex &other) const
    { return !operator==(other); }
    QPersistentModelIndex &operator=(const QPersistentModelIndex &other);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QPersistentModelIndex(QPersistentModelIndex &&other) Q_DECL_NOTHROW
        : d(other.d) { other.d = Q_NULLPTR; }
    inline QPersistentModelIndex &operator=(QPersistentModelIndex &&other) Q_DECL_NOTHROW
    { qSwap(d, other.d); return *this; }
#endif
    inline void swap(QPersistentModelIndex &other) Q_DECL_NOTHROW { qSwap(d, other.d); }
    bool operator==(const QModelIndex &other) const;
    bool operator!=(const QModelIndex &other) const;
    QPersistentModelIndex &operator=(const QModelIndex &other);
    operator const QModelIndex&() const;
    int row() const;
    int column() const;
    void *internalPointer() const;
    quintptr internalId() const;
    QModelIndex parent() const;
    QModelIndex sibling(int row, int column) const;
#if QT_DEPRECATED_SINCE(5, 8)
    QT_DEPRECATED_X("Use QAbstractItemModel::index") QModelIndex child(int row, int column) const;
#endif
    QVariant data(int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags() const;
    const QAbstractItemModel *model() const;
    bool isValid() const;
private:
    QPersistentModelIndexData *d;
    friend uint qHash(const QPersistentModelIndex &, uint seed) Q_DECL_NOTHROW;
#ifndef QT_NO_DEBUG_STREAM
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QPersistentModelIndex &);
#endif
};
Q_DECLARE_SHARED(QPersistentModelIndex)

inline uint qHash(const QPersistentModelIndex &index, uint seed) Q_DECL_NOTHROW
{ return qHash(index.d, seed); }


template<typename T> class QList;
typedef QList<QModelIndex> QModelIndexList;

class QMimeData;
class QAbstractItemModelPrivate;
template <class Key, class T> class QMap;


class Q_CORE_EXPORT QAbstractItemModel : public QObject
{
    Q_OBJECT

    friend class QPersistentModelIndexData;
    friend class QAbstractItemViewPrivate;
    friend class QIdentityProxyModel;
public:

    explicit QAbstractItemModel(QObject *parent = Q_NULLPTR);
    virtual ~QAbstractItemModel();

    Q_INVOKABLE bool hasIndex(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    Q_INVOKABLE virtual QModelIndex index(int row, int column,
                              const QModelIndex &parent = QModelIndex()) const = 0;
    Q_INVOKABLE virtual QModelIndex parent(const QModelIndex &child) const = 0;

    Q_INVOKABLE virtual QModelIndex sibling(int row, int column, const QModelIndex &idx) const;
    Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const = 0;
    Q_INVOKABLE virtual int columnCount(const QModelIndex &parent = QModelIndex()) const = 0;
    Q_INVOKABLE virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

    Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const = 0;
    Q_INVOKABLE virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    Q_INVOKABLE virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                               int role = Qt::EditRole);

    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const;
    virtual bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles);

    virtual QStringList mimeTypes() const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int column, const QModelIndex &parent) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                              int row, int column, const QModelIndex &parent);
    virtual Qt::DropActions supportedDropActions() const;

    virtual Qt::DropActions supportedDragActions() const;
#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED void setSupportedDragActions(Qt::DropActions actions)
    { doSetSupportedDragActions(actions); }
#endif

    virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    virtual bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex());
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    virtual bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex());
    virtual bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                          const QModelIndex &destinationParent, int destinationChild);
    virtual bool moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count,
                             const QModelIndex &destinationParent, int destinationChild);

    inline bool insertRow(int row, const QModelIndex &parent = QModelIndex());
    inline bool insertColumn(int column, const QModelIndex &parent = QModelIndex());
    inline bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    inline bool removeColumn(int column, const QModelIndex &parent = QModelIndex());
    inline bool moveRow(const QModelIndex &sourceParent, int sourceRow,
                        const QModelIndex &destinationParent, int destinationChild);
    inline bool moveColumn(const QModelIndex &sourceParent, int sourceColumn,
                           const QModelIndex &destinationParent, int destinationChild);

    Q_INVOKABLE virtual void fetchMore(const QModelIndex &parent);
    Q_INVOKABLE virtual bool canFetchMore(const QModelIndex &parent) const;
    Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    virtual QModelIndex buddy(const QModelIndex &index) const;
    Q_INVOKABLE virtual QModelIndexList match(const QModelIndex &start, int role,
                                              const QVariant &value, int hits = 1,
                                              Qt::MatchFlags flags =
                                              Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap)) const;
    virtual QSize span(const QModelIndex &index) const;

    virtual QHash<int,QByteArray> roleNames() const;

    using QObject::parent;

    enum LayoutChangeHint
    {
        NoLayoutChangeHint,
        VerticalSortHint,
        HorizontalSortHint
    };
    Q_ENUM(LayoutChangeHint)

Q_SIGNALS:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void headerDataChanged(Qt::Orientation orientation, int first, int last);
    void layoutChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void layoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);

    void rowsAboutToBeInserted(const QModelIndex &parent, int first, int last, QPrivateSignal);
    void rowsInserted(const QModelIndex &parent, int first, int last, QPrivateSignal);

    void rowsAboutToBeRemoved(const QModelIndex &parent, int first, int last, QPrivateSignal);
    void rowsRemoved(const QModelIndex &parent, int first, int last, QPrivateSignal);

    void columnsAboutToBeInserted(const QModelIndex &parent, int first, int last, QPrivateSignal);
    void columnsInserted(const QModelIndex &parent, int first, int last, QPrivateSignal);

    void columnsAboutToBeRemoved(const QModelIndex &parent, int first, int last, QPrivateSignal);
    void columnsRemoved(const QModelIndex &parent, int first, int last, QPrivateSignal);

    void modelAboutToBeReset(QPrivateSignal);
    void modelReset(QPrivateSignal);

    void rowsAboutToBeMoved( const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow, QPrivateSignal);
    void rowsMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row, QPrivateSignal);

    void columnsAboutToBeMoved( const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationColumn, QPrivateSignal);
    void columnsMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int column, QPrivateSignal);

public Q_SLOTS:
    virtual bool submit();
    virtual void revert();

protected Q_SLOTS:
    // Qt 6: Make virtual
    void resetInternalData();

protected:
    QAbstractItemModel(QAbstractItemModelPrivate &dd, QObject *parent = Q_NULLPTR);

    inline QModelIndex createIndex(int row, int column, void *data = Q_NULLPTR) const;
    inline QModelIndex createIndex(int row, int column, quintptr id) const;

    void encodeData(const QModelIndexList &indexes, QDataStream &stream) const;
    bool decodeData(int row, int column, const QModelIndex &parent, QDataStream &stream);

    void beginInsertRows(const QModelIndex &parent, int first, int last);
    void endInsertRows();

    void beginRemoveRows(const QModelIndex &parent, int first, int last);
    void endRemoveRows();

    bool beginMoveRows(const QModelIndex &sourceParent, int sourceFirst, int sourceLast, const QModelIndex &destinationParent, int destinationRow);
    void endMoveRows();

    void beginInsertColumns(const QModelIndex &parent, int first, int last);
    void endInsertColumns();

    void beginRemoveColumns(const QModelIndex &parent, int first, int last);
    void endRemoveColumns();

    bool beginMoveColumns(const QModelIndex &sourceParent, int sourceFirst, int sourceLast, const QModelIndex &destinationParent, int destinationColumn);
    void endMoveColumns();


#if QT_DEPRECATED_SINCE(5,0)
    QT_DEPRECATED void reset()
    {
        beginResetModel();
        endResetModel();
    }
#endif

    void beginResetModel();
    void endResetModel();

    void changePersistentIndex(const QModelIndex &from, const QModelIndex &to);
    void changePersistentIndexList(const QModelIndexList &from, const QModelIndexList &to);
    QModelIndexList persistentIndexList() const;

#if QT_DEPRECATED_SINCE(5,0)
    QT_DEPRECATED void setRoleNames(const QHash<int,QByteArray> &theRoleNames)
    {
        doSetRoleNames(theRoleNames);
    }
#endif

private:
    void doSetRoleNames(const QHash<int,QByteArray> &roleNames);
    void doSetSupportedDragActions(Qt::DropActions actions);

    Q_DECLARE_PRIVATE(QAbstractItemModel)
    Q_DISABLE_COPY(QAbstractItemModel)
};

inline bool QAbstractItemModel::insertRow(int arow, const QModelIndex &aparent)
{ return insertRows(arow, 1, aparent); }
inline bool QAbstractItemModel::insertColumn(int acolumn, const QModelIndex &aparent)
{ return insertColumns(acolumn, 1, aparent); }
inline bool QAbstractItemModel::removeRow(int arow, const QModelIndex &aparent)
{ return removeRows(arow, 1, aparent); }
inline bool QAbstractItemModel::removeColumn(int acolumn, const QModelIndex &aparent)
{ return removeColumns(acolumn, 1, aparent); }
inline bool QAbstractItemModel::moveRow(const QModelIndex &sourceParent, int sourceRow,
                                        const QModelIndex &destinationParent, int destinationChild)
{ return moveRows(sourceParent, sourceRow, 1, destinationParent, destinationChild); }
inline bool QAbstractItemModel::moveColumn(const QModelIndex &sourceParent, int sourceColumn,
                                           const QModelIndex &destinationParent, int destinationChild)
{ return moveColumns(sourceParent, sourceColumn, 1, destinationParent, destinationChild); }
inline QModelIndex QAbstractItemModel::createIndex(int arow, int acolumn, void *adata) const
{ return QModelIndex(arow, acolumn, adata, this); }
inline QModelIndex QAbstractItemModel::createIndex(int arow, int acolumn, quintptr aid) const
{ return QModelIndex(arow, acolumn, aid, this); }

class Q_CORE_EXPORT QAbstractTableModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit QAbstractTableModel(QObject *parent = Q_NULLPTR);
    ~QAbstractTableModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const Q_DECL_OVERRIDE;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) Q_DECL_OVERRIDE;

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

    using QObject::parent;

protected:
    QAbstractTableModel(QAbstractItemModelPrivate &dd, QObject *parent);

private:
    Q_DISABLE_COPY(QAbstractTableModel)
    QModelIndex parent(const QModelIndex &child) const Q_DECL_OVERRIDE;
    bool hasChildren(const QModelIndex &parent) const Q_DECL_OVERRIDE;
};

class Q_CORE_EXPORT QAbstractListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit QAbstractListModel(QObject *parent = Q_NULLPTR);
    ~QAbstractListModel();

    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const Q_DECL_OVERRIDE;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) Q_DECL_OVERRIDE;

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

    using QObject::parent;

protected:
    QAbstractListModel(QAbstractItemModelPrivate &dd, QObject *parent);

private:
    Q_DISABLE_COPY(QAbstractListModel)
    QModelIndex parent(const QModelIndex &child) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    bool hasChildren(const QModelIndex &parent) const Q_DECL_OVERRIDE;
};


inline uint qHash(const QModelIndex &index) Q_DECL_NOTHROW
{ return uint((uint(index.row()) << 4) + index.column() + index.internalId()); }

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QModelIndexList)

#endif // QABSTRACTITEMMODEL_H
