/*
 * This file is part of DSCreator.
 * Copyright (C) 2014-2019  Andrey Efremov <duxus@yandex.ru>
 *
 * DSCreator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DSCreator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DSCreator.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "script.h"
#include <QMainWindow>
#include <QSettings>
#include <QFileInfo>
#include <QListWidgetItem>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btOpenSubtitles_clicked();
    void on_btSaveCSV_clicked();
    void on_btSaveTSV_clicked();
    void on_btSaveHTML_clicked();
//    void on_lsActors_itemClicked(QListWidgetItem* item);

private:
    Ui::MainWindow *ui;
    QSettings _settings;
    QFileInfo _fileInfo;
    Script::Script _script;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void updateActors();
    QStringList getCheckedActors() const;
    QString getSaveFileName(const QStringList& actors, const QString& suffix);
    int getTimeStart() const;
    void openFile(const QString &fileName);
};

#endif // MAINWINDOW_H
