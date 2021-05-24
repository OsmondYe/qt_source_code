#ifndef QCOMPLETER_H
#define QCOMPLETER_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qstring.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qrect.h>

//QT_REQUIRE_CONFIG(completer);


class QCompleterPrivate;
class QAbstractItemView;
class QAbstractProxyModel;
class QWidget;

// oye 不是widget, 只是一个数据功能
class  QCompleter : public QObject
{
    //Q_OBJECT
//    Q_PROPERTY(QString completionPrefix READ completionPrefix WRITE setCompletionPrefix)
//    Q_PROPERTY(ModelSorting modelSorting READ modelSorting WRITE setModelSorting)
//    Q_PROPERTY(Qt::MatchFlags filterMode READ filterMode WRITE setFilterMode)
//    Q_PROPERTY(CompletionMode completionMode READ completionMode WRITE setCompletionMode)
//    Q_PROPERTY(int completionColumn READ completionColumn WRITE setCompletionColumn)
//    Q_PROPERTY(int completionRole READ completionRole WRITE setCompletionRole)
//    Q_PROPERTY(int maxVisibleItems READ maxVisibleItems WRITE setMaxVisibleItems)
//    Q_PROPERTY(Qt::CaseSensitivity caseSensitivity READ caseSensitivity WRITE setCaseSensitivity)
//    Q_PROPERTY(bool wrapAround READ wrapAround WRITE setWrapAround)

public:
    void setWidget(QWidget *widget);
    QWidget *widget() const;

    void setModel(QAbstractItemModel *c);
    QAbstractItemModel *model() const;

    void setCompletionMode(CompletionMode mode);
    CompletionMode completionMode() const;

    void setFilterMode(Qt::MatchFlags filterMode);
    Qt::MatchFlags filterMode() const;

    QAbstractItemView *popup() const;
    void setPopup(QAbstractItemView *popup);

    void setCaseSensitivity(Qt::CaseSensitivity caseSensitivity);
    Qt::CaseSensitivity caseSensitivity() const;

    void setModelSorting(ModelSorting sorting);
    ModelSorting modelSorting() const;

    void setCompletionColumn(int column);
    int  completionColumn() const;

    void setCompletionRole(int role);
    int  completionRole() const;

    bool wrapAround() const;

    int maxVisibleItems() const;
    void setMaxVisibleItems(int maxItems);

    int completionCount() const;
    bool setCurrentRow(int row);
    int currentRow() const;

    QModelIndex currentIndex() const;
    QString currentCompletion() const;

    QAbstractItemModel *completionModel() const;

    QString completionPrefix() const;
	
public:
	QCompleter(QObject *parent = Q_NULLPTR);
	QCompleter(QAbstractItemModel *model, QObject *parent = Q_NULLPTR);
	QCompleter(const QStringList& completions, QObject *parent = Q_NULLPTR);
	~QCompleter();

public Q_SLOTS:
    void setCompletionPrefix(const QString &prefix);
    void complete(const QRect& rect = QRect());
    void setWrapAround(bool wrap);

public:
    virtual QString pathFromIndex(const QModelIndex &index) const;
    virtual QStringList splitPath(const QString &path) const;

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    bool event(QEvent *) override;

Q_SIGNALS:
    void activated(const QString &text);
    void activated(const QModelIndex &index);
    void highlighted(const QString &text);
    void highlighted(const QModelIndex &index);

private:
    //Q_DISABLE_COPY(QCompleter)
    //Q_DECLARE_PRIVATE(QCompleter)

    //Q_PRIVATE_SLOT(d_func(), void _q_complete(QModelIndex))
    //Q_PRIVATE_SLOT(d_func(), void _q_completionSelected(const QItemSelection&))
    //Q_PRIVATE_SLOT(d_func(), void _q_autoResizePopup())
    //Q_PRIVATE_SLOT(d_func(), void _q_fileSystemModelDirectoryLoaded(const QString&))
public:
    enum CompletionMode {
        PopupCompletion,
        UnfilteredPopupCompletion,
        InlineCompletion
    };

    enum ModelSorting {
        UnsortedModel = 0,
        CaseSensitivelySortedModel,
        CaseInsensitivelySortedModel
    };

};

#endif // QCOMPLETER_H
