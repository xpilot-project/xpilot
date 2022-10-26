#ifndef CLIPBOARDADAPTER_H
#define CLIPBOARDADAPTER_H

#include <QGuiApplication>
#include <QClipboard>
#include <QObject>

class ClipboardAdapter : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardAdapter(QObject *parent = 0) : QObject(parent) {
        clipboard = QGuiApplication::clipboard();
    }

    Q_INVOKABLE void setText(QString text){
        clipboard->setText(text, QClipboard::Clipboard);
    }
    Q_INVOKABLE QString getText(){
        return clipboard->text();
    }

private:
    QClipboard* clipboard;
};

#endif // CLIPBOARDADAPTER_H
