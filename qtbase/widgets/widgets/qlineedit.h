#ifndef QLINEEDIT_H
#define QLINEEDIT_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qframe.h>
#include <QtGui/qtextcursor.h>
#include <QtCore/qstring.h>
#include <QtCore/qmargins.h>

class QValidator;
class QMenu;
class QLineEditPrivate;
class QCompleter;
class QStyleOptionFrame;
class QAbstractSpinBox;
class QDateTimeEdit;
class QIcon;
class QToolButton;

class  QLineEdit : public QWidget
{
//    Q_OBJECT
//
//    Q_PROPERTY(QString inputMask READ inputMask WRITE setInputMask)
//    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged USER true)
//    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)
//    Q_PROPERTY(bool frame READ hasFrame WRITE setFrame)
//    Q_PROPERTY(EchoMode echoMode READ echoMode WRITE setEchoMode)
//    Q_PROPERTY(QString displayText READ displayText)
//    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition)
//    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
//    Q_PROPERTY(bool modified READ isModified WRITE setModified DESIGNABLE false)
//    Q_PROPERTY(bool hasSelectedText READ hasSelectedText)
//    Q_PROPERTY(QString selectedText READ selectedText)
//    Q_PROPERTY(bool dragEnabled READ dragEnabled WRITE setDragEnabled)
//    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
//    Q_PROPERTY(bool undoAvailable READ isUndoAvailable)
//    Q_PROPERTY(bool redoAvailable READ isRedoAvailable)
//    Q_PROPERTY(bool acceptableInput READ hasAcceptableInput)
//    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)
//    Q_PROPERTY(Qt::CursorMoveStyle cursorMoveStyle READ cursorMoveStyle WRITE setCursorMoveStyle)
//    Q_PROPERTY(bool clearButtonEnabled READ isClearButtonEnabled WRITE setClearButtonEnabled)
public:
    enum ActionPosition {
        LeadingPosition,
        TrailingPosition
    };
    //Q_ENUM(ActionPosition)

    explicit QLineEdit(QWidget *parent = Q_NULLPTR);
    explicit QLineEdit(const QString &, QWidget *parent = Q_NULLPTR);
    ~QLineEdit(){}
	// 子类自己来决定到底有效尺寸是多少
    QSize sizeHint() const override;    
    QSize minimumSizeHint() const override;
	
Q_SIGNALS:
    void textChanged(const QString &);
    void textEdited(const QString &);
    void cursorPositionChanged(int, int);
    void returnPressed();
    void editingFinished();
    void selectionChanged();
public Q_SLOTS:
    void setText(const QString &);
    void clear();
    void selectAll();
    void undo();
    void redo();
#ifndef QT_NO_CLIPBOARD
    void cut();
    void copy() const;
    void paste();
#endif

public:
    QString text() const;
    QString displayText() const;
    QString placeholderText() const;
    void setPlaceholderText(const QString &);

    int maxLength() const;
    void setMaxLength(int);

    void setFrame(bool);
    bool hasFrame() const;

    void setClearButtonEnabled(bool enable);
    bool isClearButtonEnabled() const;

    enum EchoMode { Normal, NoEcho, Password, PasswordEchoOnEdit };
    Q_ENUM(EchoMode)
    EchoMode echoMode() const;
    void setEchoMode(EchoMode);

    bool isReadOnly() const;
    void setReadOnly(bool);

#ifndef QT_NO_VALIDATOR
    void setValidator(const QValidator *);
    const QValidator * validator() const;
#endif

#if QT_CONFIG(completer)
    void setCompleter(QCompleter *completer);
    QCompleter *completer() const;
#endif

    int cursorPosition() const;
    void setCursorPosition(int);
    int cursorPositionAt(const QPoint &pos);

    void setAlignment(Qt::Alignment flag);
    Qt::Alignment alignment() const;

    void cursorForward(bool mark, int steps = 1);
    void cursorBackward(bool mark, int steps = 1);
    void cursorWordForward(bool mark);
    void cursorWordBackward(bool mark);
    void backspace();
    void del();
    void home(bool mark);
    void end(bool mark);

    bool isModified() const;
    void setModified(bool);

    void setSelection(int, int);
    bool hasSelectedText() const;
    QString selectedText() const;
    int selectionStart() const;

    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    void setDragEnabled(bool b);
    bool dragEnabled() const;

    void setCursorMoveStyle(Qt::CursorMoveStyle style);
    Qt::CursorMoveStyle cursorMoveStyle() const;

    QString inputMask() const;
    void setInputMask(const QString &inputMask);
    bool hasAcceptableInput() const;

    void setTextMargins(int left, int top, int right, int bottom);
    void setTextMargins(const QMargins &margins);
    void getTextMargins(int *left, int *top, int *right, int *bottom) const;
    QMargins textMargins() const;

#if QT_CONFIG(action)
    using QWidget::addAction;
    void addAction(QAction *action, ActionPosition position);
    QAction *addAction(const QIcon &icon, ActionPosition position);
#endif

public:
    void deselect();
    void insert(const QString &);
#ifndef QT_NO_CONTEXTMENU
    QMenu *createStandardContextMenu();
#endif

protected: //event
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void paintEvent(QPaintEvent *) override;
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dropEvent(QDropEvent *) override;
#endif
    void changeEvent(QEvent *) override;
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *) override;
#endif

    void inputMethodEvent(QInputMethodEvent *) override;
	// oye 是不是所有的widget都有这个普通成员函数,来初始化要绘制时的基本信息
    void initStyleOption(QStyleOptionFrame *option) const;
public:
    bool event(QEvent *) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery) const override;
    Q_INVOKABLE QVariant inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const;
protected:
    QRect cursorRect() const;
private:
    friend class QAbstractSpinBox;
    friend class QAccessibleLineEdit;
    friend class QComboBox;
#ifdef QT_KEYPAD_NAVIGATION
    friend class QDateTimeEdit;
#endif
    //Q_DISABLE_COPY(QLineEdit)
    //Q_DECLARE_PRIVATE(QLineEdit)
    
    Q_PRIVATE_SLOT(d_func(), void _q_handleWindowActivate())
    Q_PRIVATE_SLOT(d_func(), void _q_textEdited(const QString &))
    Q_PRIVATE_SLOT(d_func(), void _q_cursorPositionChanged(int, int))
    
#if QT_CONFIG(completer)
    Q_PRIVATE_SLOT(d_func(), void _q_completionHighlighted(const QString &))
#endif

#ifdef QT_KEYPAD_NAVIGATION
    Q_PRIVATE_SLOT(d_func(), void _q_editFocusChange(bool))
#endif

    Q_PRIVATE_SLOT(d_func(), void _q_selectionChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_updateNeeded(const QRect &))
    Q_PRIVATE_SLOT(d_func(), void _q_textChanged(const QString &))
    Q_PRIVATE_SLOT(d_func(), void _q_clearButtonClicked())
};


#endif // QLINEEDIT_H
