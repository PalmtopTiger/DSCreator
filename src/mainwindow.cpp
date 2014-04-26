#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "csvreader.h"
#include "script.h"
#include <QTextCodec>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>
#include <QMimeData>
#include <QUrl>

QString UrlToPath(const QUrl &url);

const QString DEFAULT_DIR_KEY = "DefaultDir";
const QStringList FILETYPES = QStringList() << "ass" << "ssa" << "srt" << "csv"; //! @todo: разделить?
const QString FILETYPES_FILTER = QTextCodec::codecForName("UTF-8")->toUnicode("Субтитры") + " (*." + FILETYPES.join(" *.") + ")";


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->move(QApplication::desktop()->screenGeometry().center() - this->rect().center());
}

MainWindow::~MainWindow()
{
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
            this->openFile(fileName);
        }
    }
}

void MainWindow::on_btOpen_clicked()
{
    QSettings settings;
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          "Выберите файл",
                                                          settings.value(DEFAULT_DIR_KEY).toString(),
                                                          FILETYPES_FILTER);

    if (fileName.isEmpty()) return;

    const QFileInfo fileInfo(fileName);
    settings.setValue(DEFAULT_DIR_KEY, QFileInfo(fileName).absoluteDir().path());

    this->openFile(fileName);
}

void MainWindow::on_btSave_clicked()
{
    QString templateName = this->fileInfo.path() + QDir::separator() + this->fileInfo.baseName();
    if (!this->checkedStyles.isEmpty()) templateName.append(" (" + this->checkedStyles.join(",") + ')');
    templateName.append(".csv");

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          "Выберите файл",
                                                          QDir(templateName).path(),
                                                          "CSV (*.csv)");

    if (fileName.isEmpty()) return;

    this->saveCSV(fileName);
}

void MainWindow::on_lstStyles_itemChanged(QListWidgetItem *item)
{
    Q_UNUSED(item);

    this->checkedStyles.clear();
    for (int i = 0; i < ui->lstStyles->count(); ++i)
    {
        const QListWidgetItem* const item = ui->lstStyles->item(i);
        if (Qt::Checked == item->checkState()) this->checkedStyles.append(item->text());
    }
}

void MainWindow::on_btRenumber_clicked()
{
    if (ui->edPrefix->text().isEmpty())
    {
        QMessageBox::warning(this, "Предупреждение", "Возможно, вы забыли ввести префикс кода.");
    }

    this->renumber();
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

void MainWindow::openFile(const QString &fileName)
{
    // Очистка
    ui->btSave->setEnabled(false);
    ui->btRenumber->setEnabled(false);
    this->fileInfo.setFile(fileName);
    this->data.clear();

    // Определение номера эпизода
    const QRegExp episodeNumber("S\\d+E\\d+", Qt::CaseInsensitive);
    if (episodeNumber.indexIn(this->fileInfo.baseName()) >= 0)
    {
        QStringList parts;
        if (!ui->edPrefix->text().isEmpty())
        {
            QString temp = ui->edPrefix->text();
            temp.remove(QRegExp("_?S\\d+E\\d+$", Qt::CaseInsensitive));
            if (!temp.isEmpty()) parts.append(temp);
        }
        parts.append(episodeNumber.cap(0).toUpper());
        ui->edPrefix->setText(parts.join("_"));
    }

    if (this->fileInfo.suffix().toLower() == "csv")
    {
        if (!this->openCSV(fileName)) return;
    }
    else
    {
        if (!this->openSubtitles(fileName)) return;
    }

    this->updateStyles();
    ui->btSave->setEnabled(true);
    ui->btRenumber->setEnabled(true);
}

bool MainWindow::openSubtitles(const QString &fileName)
{
    // Чтение файла
    QFile fin(fileName);
    if ( !fin.open(QFile::ReadOnly | QFile::Text) )
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка открытия файла");
        return false;
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
            return false;
        }
        break;

    case Script::SCR_SRT:
        if ( !Script::ParseSRT(in, script) )
        {
            QMessageBox::critical(this, "Ошибка", "Файл не соответствует формату SRT");
            fin.close();
            return false;
        }
        break;

    default:
        QMessageBox::critical(this, "Ошибка", "Неизвестный формат файла");
        fin.close();
        return false;
    }
    fin.close();

    const QRegExp endLineTag("\\\\n", Qt::CaseInsensitive), assTags("\\{[^\\}]*\\}", Qt::CaseInsensitive);
    QList<QStandardItem*> row;
    foreach (const Script::Line::Event* event, script.events.content)
    {
        row.append(new QStandardItem("-"));
        row.append(new QStandardItem( Script::Line::TimeToStr(event->start, Script::SCR_ASS) ));
        row.append(new QStandardItem( event->style.trimmed() ));
        row.append(new QStandardItem( event->text.trimmed().replace(endLineTag, " ").replace(assTags, QString::null) ));

        this->data.appendRow(row);
        row.clear();
    }

    // Определение единых фраз
    const QRegExp phraseNotBegin("^\\W*[a-zа-яё]"), phraseNotEnd("[^.?!…]$");
    for (int row = this->data.rowCount() - 1; row > 0; --row)
    {
        const int prev_row = row - 1;
        if ( this->data.item(row, 2)->text() == this->data.item(prev_row, 2)->text() &&
             ( this->data.item(row, 3)->text().contains(phraseNotBegin) || this->data.item(prev_row, 3)->text().contains(phraseNotEnd) ) )
        {
            this->data.item(prev_row, 3)->setText( this->data.item(prev_row, 3)->text() + " " + this->data.item(row, 3)->text() );
            this->data.removeRow(row);
        }
    }

    return true;
}

bool MainWindow::openCSV(const QString &fileName)
{
    // Чтение файла
    CSVReader reader;
    if (!reader.read(fileName, &this->data))
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка открытия файла");
        return false;
    }

    if (this->data.columnCount() != 4)
    {
        QMessageBox::critical(this, "Ошибка", "Файл не соответствует принятому формату");
        return false;
    }

    return true;
}

void MainWindow::saveCSV(const QString &fileName)
{
    QFile fout(fileName);
    if ( !fout.open(QFile::WriteOnly | QFile::Text) )
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка сохранения файла");
        return;
    }

    QTextStream out(&fout);
    out.setCodec( QTextCodec::codecForName("UTF-8") );
//    out.setGenerateByteOrderMark(true);

    if (ui->chkHeader->isChecked()) out << QString("Код,Время,Стиль,Текст\n");

    const QChar separator = ',';
    QString temp;
    QStringList tempLine;
    for (int row = 0; row < this->data.rowCount(); ++row)
    {
        if ( this->checkedStyles.isEmpty() || this->checkedStyles.contains(this->data.item(row, 2)->text(), Qt::CaseInsensitive) )
        {
            for (int col = 0; col < this->data.columnCount(); ++col)
            {
                temp = this->data.item(row, col)->text();
                if ( temp.contains(separator) )
                {
                    temp = QString("\"%1\"").arg( temp.replace(QChar('"'), "\"\"") );
                }
                tempLine.append(temp);
            }
            out << tempLine.join(separator) << QString("\n");
            tempLine.clear();
        }
    }

    fout.close();
}

void MainWindow::updateStyles()
{
    ui->lstStyles->clear();

    QSet<QString> names;
    for (int row = 0; row < this->data.rowCount(); ++row)
    {
        names.insert(this->data.item(row, 2)->text());
    }
    QStringList sortedNames = names.values();
    sortedNames.sort();
    foreach (const QString& name, sortedNames)
    {
        QListWidgetItem* item = new QListWidgetItem(name, ui->lstStyles);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

void MainWindow::renumber()
{
    const QString prefix = ui->edPrefix->text();
    const QString format = "%1-%2";
    for (int row = 0; row < this->data.rowCount(); ++row)
    {
        this->data.item(row, 0)->setText( format.arg(prefix).arg(row + 1) );
    }
}
