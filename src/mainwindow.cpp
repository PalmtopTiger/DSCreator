#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "writer.h"
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QMimeData>
#include <QUrl>

QString UrlToPath(const QUrl &url);

const QString DEFAULT_DIR_KEY = "DefaultDir";
const QString FPS_KEY = "FPS";
const QString TIME_START_KEY = "TimeStart";
const QString JOIN_INTERVAL_KEY = "JoinInterval";
const QStringList FILETYPES = QStringList() << "ass" << "ssa" << "srt";
const QString FILETYPES_FILTER = "Субтитры (*." + FILETYPES.join(" *.") + ")";


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

    this->move(QApplication::desktop()->screenGeometry().center() - this->rect().center());
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
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        const QString path = UrlToPath(event->mimeData()->urls().first());
        if (!path.isEmpty())
        {
            this->open(path);
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

    this->open(fileName);
}

void MainWindow::on_btSaveCSV_clicked()
{
    const QStringList actors = getCheckedActors();
    QString templateName = _fileInfo.path() + QDir::separator() + _fileInfo.baseName();
    if (!actors.isEmpty()) templateName.append(" (" + actors.join(",") + ')');
    templateName.append(".csv");

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          "Выберите файл",
                                                          QDir(templateName).path(),
                                                          "CSV (*.csv)");
    if (fileName.isEmpty()) return;

    if (Writer::SaveSV(_script,
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
    const QStringList actors = getCheckedActors();
    QString templateName = _fileInfo.path() + QDir::separator() + _fileInfo.baseName();
    if (!actors.isEmpty()) templateName.append(" (" + actors.join(",") + ')');
    templateName.append(".tsv");

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          "Выберите файл",
                                                          QDir(templateName).path(),
                                                          "TSV (*.tsv)");
    if (fileName.isEmpty()) return;

    if (Writer::SaveSV(_script,
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

void MainWindow::on_btSavePDF_clicked()
{
    const QStringList actors = getCheckedActors();
    QString templateName = _fileInfo.path() + QDir::separator() + _fileInfo.baseName();
    if (!actors.isEmpty()) templateName.append(" (" + actors.join(",") + ')');
    templateName.append(".pdf");

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          "Выберите файл",
                                                          QDir(templateName).path(),
                                                          "PDF (*.pdf)");
    if (fileName.isEmpty()) return;

    Writer::SavePDF(_script,
                    fileName,
                    actors,
                    ui->edFPS->value(),
                    this->getTimeStart(),
                    ui->edJoinInterval->time().msecsSinceStartOfDay());
}


//
// Функции
//
QString UrlToPath(const QUrl &url)
{
    const QString path = url.toLocalFile();

    if (!path.isEmpty() && FILETYPES.contains(QFileInfo(path).suffix(), Qt::CaseInsensitive))
    {
        return path;
    }

    return QString::null;
}

void MainWindow::updateActors()
{
    ui->lstActors->clear();

    QSet<QString> uniqueActors;
    for (const Script::Line::Event* const event : qAsConst(_script.events.content))
    {
        uniqueActors.insert(event->actorName); // Already trimmed
    }

    QStringList actors = uniqueActors.values();
    actors.sort();

    for (const QString& actor : qAsConst(actors))
    {
        QListWidgetItem* item = new QListWidgetItem(actor, ui->lstActors);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

QStringList MainWindow::getCheckedActors() const
{
    QStringList actors;
    for (int i = 0; i < ui->lstActors->count(); ++i)
    {
        const QListWidgetItem* const item = ui->lstActors->item(i);
        if (Qt::Checked == item->checkState()) actors.append(item->text());
    }
    return actors;
}

int MainWindow::getTimeStart() const
{
    const int timeStart = ui->edTimeStart->time().msecsSinceStartOfDay();

    return ui->cbNegativeTimeStart->isChecked() ? -timeStart : timeStart;
}

void MainWindow::open(const QString &fileName)
{
    // Очистка
    ui->btSaveCSV->setEnabled(false);
    ui->btSaveTSV->setEnabled(false);
    ui->btSavePDF->setEnabled(false);
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
        ui->lstActors->clear();
        ui->btSaveCSV->setEnabled(false);
        ui->btSaveTSV->setEnabled(false);
        ui->btSavePDF->setEnabled(false);

        QMessageBox::warning(this, "Сообщение", "В субтитрах нет фраз");
    }
    else
    {
        this->updateActors();
        ui->btSaveCSV->setEnabled(true);
        ui->btSaveTSV->setEnabled(true);
        ui->btSavePDF->setEnabled(true);
    }
}
