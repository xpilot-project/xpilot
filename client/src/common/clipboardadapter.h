/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2024 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
*/

#ifndef CLIPBOARDADAPTER_H
#define CLIPBOARDADAPTER_H

#include <QGuiApplication>
#include <QClipboard>
#include <QObject>
#include <QRegularExpression>

class ClipboardAdapter : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardAdapter(QObject *parent = 0) : QObject(parent) {
        clipboard = QGuiApplication::clipboard();
    }

    Q_INVOKABLE void setText(QString text){
        static QRegularExpression htmlTags("<[^>]*>");
        text.remove(htmlTags);
        clipboard->setText(text, QClipboard::Clipboard);
    }
    Q_INVOKABLE QString getText(){
        return clipboard->text();
    }

private:
    QClipboard* clipboard;
};

#endif // CLIPBOARDADAPTER_H
