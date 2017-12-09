#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "script.h"
#include <QTextCodec>
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

    ui->edFPS->setValue(_settings.value(FPS_KEY, ui->edFPS->value()).toDouble());
    ui->edTimeStart->setTime(QTime::fromMSecsSinceStartOfDay(_settings.value(TIME_START_KEY, ui->edTimeStart->time().msecsSinceStartOfDay()).toInt()));
    ui->edJoinInterval->setTime(QTime::fromMSecsSinceStartOfDay(_settings.value(JOIN_INTERVAL_KEY, ui->edJoinInterval->time().msecsSinceStartOfDay()).toInt()));

    this->move(QApplication::desktop()->screenGeometry().center() - this->rect().center());
}

MainWindow::~MainWindow()
{
    _settings.setValue(FPS_KEY, ui->edFPS->value());
    _settings.setValue(TIME_START_KEY, ui->edTimeStart->time().msecsSinceStartOfDay());
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
        const QString fileName = UrlToPath(event->mimeData()->urls().first());
        if (!fileName.isEmpty())
        {
            this->open(fileName);
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
    QString templateName = _fileInfo.path() + QDir::separator() + _fileInfo.baseName();
    if (!_checkedActors.isEmpty()) templateName.append(" (" + _checkedActors.join(",") + ')');
    templateName.append(".csv");

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          "Выберите файл",
                                                          QDir(templateName).path(),
                                                          "CSV (*.csv)");

    if (fileName.isEmpty()) return;

    this->save(fileName, FMT_CSV);
}

void MainWindow::on_btSaveTSV_clicked()
{
    QString templateName = _fileInfo.path() + QDir::separator() + _fileInfo.baseName();
    if (!_checkedActors.isEmpty()) templateName.append(" (" + _checkedActors.join(",") + ')');
    templateName.append(".tsv");

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          "Выберите файл",
                                                          QDir(templateName).path(),
                                                          "TSV (*.tsv)");

    if (fileName.isEmpty()) return;

    this->save(fileName, FMT_TSV);
}

void MainWindow::on_btSavePDF_clicked()
{
    QString templateName = _fileInfo.path() + QDir::separator() + _fileInfo.baseName();
    if (!_checkedActors.isEmpty()) templateName.append(" (" + _checkedActors.join(",") + ')');
    templateName.append(".pdf");

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          "Выберите файл",
                                                          QDir(templateName).path(),
                                                          "PDF (*.pdf)");

    if (fileName.isEmpty()) return;

    _table.toPDF(fileName,
                 _checkedActors,
                 ui->edFPS->value(),
                 ui->edTimeStart->time().msecsSinceStartOfDay(),
                 ui->edJoinInterval->time().msecsSinceStartOfDay());
}

void MainWindow::on_lstActors_itemChanged(QListWidgetItem *item)
{
    Q_UNUSED(item);

    _checkedActors.clear();
    for (int i = 0; i < ui->lstActors->count(); ++i)
    {
        const QListWidgetItem* const item = ui->lstActors->item(i);
        if (Qt::Checked == item->checkState()) _checkedActors.append(item->text());
    }
}


//
// Функции
//
QString UrlToPath(const QUrl &url)
{
    QString path = url.toLocalFile();
    if (!path.isEmpty() && FILETYPES.contains(QFileInfo(path).suffix(), Qt::CaseInsensitive)) return path;

    return QString::null;
}

void MainWindow::updateActors()
{
    ui->lstActors->clear();

    QStringList actors = _table.actors();
    actors.sort();

    foreach (const QString& actor, actors)
    {
        QListWidgetItem* item = new QListWidgetItem(actor, ui->lstActors);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

void MainWindow::open(const QString &fileName)
{
    // Очистка
    ui->btSaveCSV->setEnabled(false);
    ui->btSaveTSV->setEnabled(false);
    ui->btSavePDF->setEnabled(false);
    _fileInfo.setFile(fileName);

    // Чтение файла
    QFile fin(fileName);
    if ( !fin.open(QFile::ReadOnly | QFile::Text) )
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка открытия файла");
        return;
    }

    Script::Script script;
    QTextStream in(&fin);
    switch (Script::DetectFormat(in))
    {
    case Script::SCR_SSA:
    case Script::SCR_ASS:
        if ( !Script::ParseSSA(in, script) )
        {
            QMessageBox::critical(this, "Ошибка", "Файл не соответствует формату SSA/ASS");
            fin.close();
            return;
        }
        break;

    case Script::SCR_SRT:
        if ( !Script::ParseSRT(in, script) )
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

    _table = script;

    this->updateActors();
    ui->btSaveCSV->setEnabled(true);
    ui->btSaveTSV->setEnabled(true);
    ui->btSavePDF->setEnabled(true);
}

void MainWindow::save(const QString &fileName, const Format format)
{
    QFile fout(fileName);
    if ( !fout.open(QFile::WriteOnly | QFile::Text) )
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка сохранения файла");
        return;
    }

    QTextStream out(&fout);
    out.setCodec( QTextCodec::codecForName("UTF-8") );
    out.setGenerateByteOrderMark(true);

    const double fps = ui->edFPS->value();
    const int timeStart = ui->edTimeStart->time().msecsSinceStartOfDay();
    const int joinInterval = ui->edJoinInterval->time().msecsSinceStartOfDay();
    switch (format)
    {
    case FMT_CSV:
        out << _table.toCSV(_checkedActors, fps, timeStart, joinInterval);
        break;

    case FMT_TSV:
        out << _table.toTSV(_checkedActors, fps, timeStart, joinInterval);
        break;

    default:
        QMessageBox::critical(this, "Ошибка", "Неизвестный формат файла");
    }

    fout.close();
}
