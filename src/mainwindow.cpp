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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "writer.h"
#include <QStyle>
#include <QScreen>
#include <QDragEnterEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QMimeData>
#include <QUrl>

QString UrlToPath(const QUrl &url);

const QStringList FILETYPES = {"ass", "ssa", "srt"};
const QString FILETYPES_FILTER  = QString("Субтитры (*.%1)").arg(FILETYPES.join(" *.")),
              DEFAULT_DIR_KEY   = "DefaultDir",
              FPS_KEY           = "FPS",
              TIME_START_KEY    = "TimeStart",
              JOIN_INTERVAL_KEY = "JoinInterval";


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    const int timeStart = _settings.value(TIME_START_KEY, this->getTimeStart()).toInt();

    ui->edFPS->setValue(_settings.value(FPS_KEY, ui->edFPS->value()).toDouble());
    ui->cbNegativeTimeStart->setChecked(timeStart < 0);
    ui->edTimeStart->setTime(QTime::fromMSecsSinceStartOfDay(abs(timeStart)));
    ui->edJoinInterval->setTime(QTime::fromMSecsSinceStartOfDay(_settings.value(JOIN_INTERVAL_KEY, ui->edJoinInterval->time().msecsSinceStartOfDay()).toInt()));

    this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, this->size(), qApp->primaryScreen()->availableGeometry()));
}

MainWindow::~MainWindow()
{
    _settings.setValue(FPS_KEY, ui->edFPS->value());
    _settings.setValue(TIME_START_KEY, this->getTimeStart());
    _settings.setValue(JOIN_INTERVAL_KEY, ui->edJoinInterval->time().msecsSinceStartOfDay());

    delete ui;
}


//
// Слоты
//
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && !UrlToPath(event->mimeData()->urls().first()).isEmpty())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        const QString path = UrlToPath(event->mimeData()->urls().first());
        if (!path.isEmpty())
        {
            this->openFile(path);
            event->acceptProposedAction();
        }
    }
}

void MainWindow::on_btOpenSubtitles_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          "Выберите файл",
                                                          _settings.value(DEFAULT_DIR_KEY).toString(),
                                                          FILETYPES_FILTER);

    if (fileName.isEmpty()) return;

    _settings.setValue(DEFAULT_DIR_KEY, QFileInfo(fileName).absolutePath());

    this->openFile(fileName);
}

void MainWindow::on_btSaveCSV_clicked()
{
    const QStringList actors = this->getCheckedActors();
    const QString fileName   = this->getSaveFileName(actors, "csv");
    if (fileName.isEmpty()) return;

    if (!Writer::SaveSV(_script,
                        fileName,
                        actors,
                        ui->edFPS->value(),
                        this->getTimeStart(),
                        ui->edJoinInterval->time().msecsSinceStartOfDay(),
                        Writer::SEP_CSV))
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка сохранения файла");
    }
}

void MainWindow::on_btSaveTSV_clicked()
{
    const QStringList actors = this->getCheckedActors();
    const QString fileName   = this->getSaveFileName(actors, "tsv");
    if (fileName.isEmpty()) return;

    if (!Writer::SaveSV(_script,
                        fileName,
                        actors,
                        ui->edFPS->value(),
                        this->getTimeStart(),
                        ui->edJoinInterval->time().msecsSinceStartOfDay(),
                        Writer::SEP_TSV))
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка сохранения файла");
    }
}

void MainWindow::on_btSaveHTML_clicked()
{
    const QStringList actors = this->getCheckedActors();
    const QString fileName   = this->getSaveFileName(actors, "html");
    if (fileName.isEmpty()) return;

    if (!Writer::SaveHTML(_script,
                          fileName,
                          actors,
                          ui->edFPS->value(),
                          this->getTimeStart(),
                          ui->edJoinInterval->time().msecsSinceStartOfDay(),
                          _fileInfo.completeBaseName()))
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка сохранения файла");
    }
}

/*void MainWindow::on_lsActors_itemClicked(QListWidgetItem* item)
{
    if (nullptr == item) return;
    item->setCheckState(Qt::Checked == item->checkState() ? Qt::Unchecked : Qt::Checked);
}*/


//
// Функции
//
QString UrlToPath(const QUrl &url)
{
    if (url.isLocalFile()) {
        const QString path = url.toLocalFile();
        if (FILETYPES.contains(QFileInfo(path).suffix(), Qt::CaseInsensitive)) {
            return path;
        }
    }
    return QString();
}

void MainWindow::updateActors()
{
    ui->lsActors->clear();

    QSet<QString> uniqueActors;
    for (const Script::Line::Event* const event : qAsConst(_script.events.content))
    {
        uniqueActors.insert(event->actorName.isEmpty() ? Writer::ACTOR_EMPTY : event->actorName); // Already trimmed
    }

    QStringList actors = uniqueActors.values();
    actors.sort();

    for (const QString& actor : qAsConst(actors))
    {
        QListWidgetItem* const item = new QListWidgetItem(actor, ui->lsActors);
//        item->setFlags(item->flags() & (~Qt::ItemIsUserCheckable));
        item->setCheckState(Qt::Unchecked);
    }
}

QStringList MainWindow::getCheckedActors() const
{
    QStringList actors;
    for (int i = 0; i < ui->lsActors->count(); ++i)
    {
        const QListWidgetItem* const item = ui->lsActors->item(i);
        if (Qt::Checked == item->checkState()) actors.append(item->text());
    }
    return actors;
}

QString MainWindow::getSaveFileName(const QStringList& actors, const QString& suffix)
{
    QString fileName = _fileInfo.completeBaseName();
    if (!actors.isEmpty()) fileName.append(QString(" (%1)").arg(actors.join(',')));
    fileName.append(QString(".%1").arg(suffix));

    return QFileDialog::getSaveFileName(this,
                                        "Выберите файл",
                                        _fileInfo.dir().filePath(fileName),
                                        QString("%1 (*.%2)").arg(suffix.toUpper()).arg(suffix));
}

int MainWindow::getTimeStart() const
{
    const int timeStart = ui->edTimeStart->time().msecsSinceStartOfDay();

    return ui->cbNegativeTimeStart->isChecked() ? -timeStart : timeStart;
}

void MainWindow::openFile(const QString &fileName)
{
    // Очистка
    ui->lsActors->setEnabled(false);
    ui->btSaveCSV->setEnabled(false);
    ui->btSaveTSV->setEnabled(false);
    ui->btSaveHTML->setEnabled(false);
    _fileInfo.setFile(fileName);
    _script.clear();

    // Чтение файла
    QFile fin(fileName);
    if ( !fin.open(QFile::ReadOnly | QFile::Text) )
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка открытия файла");
        return;
    }

    QTextStream in(&fin);
    switch (Script::DetectFormat(in))
    {
    case Script::SCR_SSA:
    case Script::SCR_ASS:
        if ( !Script::ParseSSA(in, _script) )
        {
            QMessageBox::critical(this, "Ошибка", "Файл не соответствует формату SSA/ASS");
            fin.close();
            return;
        }
        break;

    case Script::SCR_SRT:
        if ( !Script::ParseSRT(in, _script) )
        {
            QMessageBox::critical(this, "Ошибка", "Файл не соответствует формату SRT");
            fin.close();
            return;
        }
        break;

    default:
        QMessageBox::critical(this, "Ошибка", "Неизвестный формат файла");
        fin.close();
        return;
    }
    fin.close();

    if (_script.events.content.isEmpty())
    {
        ui->lsActors->clear();
        QMessageBox::warning(this, "Сообщение", "В субтитрах нет фраз");
    }
    else
    {
        this->updateActors();
        ui->lsActors->setEnabled(true);
        ui->btSaveCSV->setEnabled(true);
        ui->btSaveTSV->setEnabled(true);
        ui->btSaveHTML->setEnabled(true);
    }
}
