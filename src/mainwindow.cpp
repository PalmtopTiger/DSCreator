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
const QStringList FILETYPES = QStringList() << "ass" << "ssa" << "srt";
#if QT_VERSION >= 0x050000
const QString FILETYPES_FILTER = "Субтитры (*." + FILETYPES.join(" *.") + ")";
#else
const QString FILETYPES_FILTER = QTextCodec::codecForName("UTF-8")->toUnicode("Субтитры") + " (*." + FILETYPES.join(" *.") + ")";
#endif


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->edFPS->setValue(_settings.value(FPS_KEY, ui->edFPS->value()).toDouble());

    this->move(QApplication::desktop()->screenGeometry().center() - this->rect().center());
}

MainWindow::~MainWindow()
{
    _settings.setValue(FPS_KEY, ui->edFPS->value());

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
    if (!_checkedStyles.isEmpty()) templateName.append(" (" + _checkedStyles.join(",") + ')');
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
    if (!_checkedStyles.isEmpty()) templateName.append(" (" + _checkedStyles.join(",") + ')');
    templateName.append(".tsv");

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          "Выберите файл",
                                                          QDir(templateName).path(),
                                                          "TSV (*.tsv)");

    if (fileName.isEmpty()) return;

    this->save(fileName, FMT_TSV);
}

void MainWindow::on_lstStyles_itemChanged(QListWidgetItem *item)
{
    Q_UNUSED(item);

    _checkedStyles.clear();
    for (int i = 0; i < ui->lstStyles->count(); ++i)
    {
        const QListWidgetItem* const item = ui->lstStyles->item(i);
        if (Qt::Checked == item->checkState()) _checkedStyles.append(item->text());
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

void MainWindow::updateStyles()
{
    ui->lstStyles->clear();

    QStringList styles = _table.styles();
    styles.sort();

    foreach (const QString& style, styles)
    {
        QListWidgetItem* item = new QListWidgetItem(style, ui->lstStyles);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

void MainWindow::open(const QString &fileName)
{
    // Очистка
    ui->btSaveCSV->setEnabled(false);
    ui->btSaveTSV->setEnabled(false);
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

    _table.clear();

    // Построение таблицы
    const QRegExp endLineTag("\\\\n", Qt::CaseInsensitive), assTags("\\{[^\\}]*\\}", Qt::CaseInsensitive);
    Table::Row* row;
    foreach (const Script::Line::Event* event, script.events.content)
    {
        row = new Table::Row;

        row->start = event->start;
        row->end   = event->end;
        row->style = event->style.trimmed();
        row->text  = event->text.trimmed().replace(endLineTag, " ").replace(assTags, QString::null);

        _table.append(row);
    }

    _table.mergeSiblings();

    this->updateStyles();
    ui->btSaveCSV->setEnabled(true);
    ui->btSaveTSV->setEnabled(true);
}

void MainWindow::save(const QString &fileName, Format format)
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

    switch (format)
    {
    case FMT_CSV:
        out << _table.toCSV(_checkedStyles, ui->edFPS->value());
        break;

    case FMT_TSV:
        out << _table.toTSV(_checkedStyles, ui->edFPS->value());
        break;

    default:
        QMessageBox::critical(this, "Ошибка", "Неизвестный формат файла");
    }

    fout.close();
}
