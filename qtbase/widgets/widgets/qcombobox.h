#ifndef QCOMBOBOX_H
#define QCOMBOBOX_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qabstractitemdelegate.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qvariant.h>
#include <QtGui/qvalidator.h>

class  QComboBox : public QWidget
{
public:
    explicit QComboBox(QWidget *parent = Q_NULLPTR);
    ~QComboBox();

    int maxVisibleItems() const;
    void setMaxVisibleItems(int maxItems);

    int count() const;
    void setMaxCount(int max);
    int maxCount() const;

#if QT_CONFIG(completer)
    bool autoCompletion() const;
    void setAutoCompletion(bool enable);

    Qt::CaseSensitivity autoCompletionCaseSensitivity() const;
    void setAutoCompletionCaseSensitivity(Qt::CaseSensitivity sensitivity);
#endif

    bool duplicatesEnabled() const;
    void setDuplicatesEnabled(bool enable);

    void setFrame(bool);
    bool hasFrame() const;

    inline int findText(const QString &text,
                        Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly|Qt::MatchCaseSensitive)) const
        { return findData(text, Qt::DisplayRole, flags); }
    int findData(const QVariant &data, int role = Qt::UserRole,
                 Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly|Qt::MatchCaseSensitive)) const;

    enum InsertPolicy {
        NoInsert,
        InsertAtTop,
        InsertAtCurrent,
        InsertAtBottom,
        InsertAfterCurrent,
        InsertBeforeCurrent,
        InsertAlphabetically
    };
    //Q_ENUM(InsertPolicy)

    InsertPolicy insertPolicy() const;
    void setInsertPolicy(InsertPolicy policy);

    enum SizeAdjustPolicy {
        AdjustToContents,
        AdjustToContentsOnFirstShow,
        AdjustToMinimumContentsLength, // ### Qt 6: remove
        AdjustToMinimumContentsLengthWithIcon
    };
    //Q_ENUM(SizeAdjustPolicy)

    SizeAdjustPolicy sizeAdjustPolicy() const;
    void setSizeAdjustPolicy(SizeAdjustPolicy policy);
    int minimumContentsLength() const;
    void setMinimumContentsLength(int characters);
    QSize iconSize() const;
    void setIconSize(const QSize &size);

	// edit
    bool isEditable() const;
    void setEditable(bool editable);
    void setLineEdit(QLineEdit *edit);
    QLineEdit *lineEdit() const;
	
#ifndef QT_NO_VALIDATOR
    void setValidator(const QValidator *v);
    const QValidator *validator() const;
#endif

#if QT_CONFIG(completer)
    void setCompleter(QCompleter *c);
    QCompleter *completer() const;
#endif

    QAbstractItemDelegate *itemDelegate() const;
    void setItemDelegate(QAbstractItemDelegate *delegate);

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

    QModelIndex rootModelIndex() const;
    void setRootModelIndex(const QModelIndex &index);

    int modelColumn() const;
    void setModelColumn(int visibleColumn);

    int currentIndex() const;
    QString currentText() const;
    QVariant currentData(int role = Qt::UserRole) const;

    QString itemText(int index) const;
    QIcon itemIcon(int index) const;
    QVariant itemData(int index, int role = Qt::UserRole) const;

    inline void addItem(const QString &text, const QVariant &userData = QVariant());
    inline void addItem(const QIcon &icon, const QString &text,
                        const QVariant &userData = QVariant());
    inline void addItems(const QStringList &texts)
        { insertItems(count(), texts); }

    inline void insertItem(int index, const QString &text, const QVariant &userData = QVariant());
    void insertItem(int index, const QIcon &icon, const QString &text,
                    const QVariant &userData = QVariant());
    void insertItems(int index, const QStringList &texts);
    void insertSeparator(int index);

    void removeItem(int index);

    void setItemText(int index, const QString &text);
    void setItemIcon(int index, const QIcon &icon);
    void setItemData(int index, const QVariant &value, int role = Qt::UserRole);

	// ¸øËû»»¸öview?
    QAbstractItemView *view() const;
    void setView(QAbstractItemView *itemView);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    virtual void showPopup();
    virtual void hidePopup();

    bool event(QEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery) const override;
    Q_INVOKABLE QVariant inputMethodQuery(Qt::InputMethodQuery query, const QVariant &argument) const;

public Q_SLOTS:
    void clear();
    void clearEditText();
    void setEditText(const QString &text);
    void setCurrentIndex(int index);
    void setCurrentText(const QString &text);
Q_SIGNALS:
    void editTextChanged(const QString &);
    void activated(int index);
    void activated(const QString &);
    void highlighted(int index);
    void highlighted(const QString &);
    void currentIndexChanged(int index);
    void currentIndexChanged(const QString &);
    void currentTextChanged(const QString &);

protected:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void changeEvent(QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *e) override;
#endif
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *e) override;
#endif // 
    void inputMethodEvent(QInputMethodEvent *) override;
    void initStyleOption(QStyleOptionComboBox *option) const;


protected:
    QComboBox(QComboBoxPrivate &, QWidget *);

private:
    Q_DECLARE_PRIVATE(QComboBox)
    Q_DISABLE_COPY(QComboBox)
    Q_PRIVATE_SLOT(d_func(), void _q_itemSelected(const QModelIndex &item))
    Q_PRIVATE_SLOT(d_func(), void _q_emitHighlighted(const QModelIndex &))
    Q_PRIVATE_SLOT(d_func(), void _q_emitCurrentIndexChanged(const QModelIndex &index))
    Q_PRIVATE_SLOT(d_func(), void _q_editingFinished())
    Q_PRIVATE_SLOT(d_func(), void _q_returnPressed())
    Q_PRIVATE_SLOT(d_func(), void _q_resetButton())
    Q_PRIVATE_SLOT(d_func(), void _q_dataChanged(const QModelIndex &, const QModelIndex &))
    Q_PRIVATE_SLOT(d_func(), void _q_updateIndexBeforeChange())
    Q_PRIVATE_SLOT(d_func(), void _q_rowsInserted(const QModelIndex & parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_rowsRemoved(const QModelIndex & parent, int start, int end))
    Q_PRIVATE_SLOT(d_func(), void _q_modelDestroyed())
    Q_PRIVATE_SLOT(d_func(), void _q_modelReset())
#if QT_CONFIG(completer)
    Q_PRIVATE_SLOT(d_func(), void _q_completerActivated(const QModelIndex &index))
#endif
};

inline void QComboBox::addItem(const QString &atext, const QVariant &auserData)
{ insertItem(count(), atext, auserData); }
inline void QComboBox::addItem(const QIcon &aicon, const QString &atext,
                               const QVariant &auserData)
{ insertItem(count(), aicon, atext, auserData); }

inline void QComboBox::insertItem(int aindex, const QString &atext,
                                  const QVariant &auserData)
{ insertItem(aindex, QIcon(), atext, auserData); }

#endif // QCOMBOBOX_H
