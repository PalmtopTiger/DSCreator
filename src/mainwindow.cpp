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
#include <QUrl>

QString UrlToPath(const QUrl &url);

const QString DEFAULT_DIR_KEY = "DefaultDir";
const QStringList FILETYPES = QStringList() << "ass" << "ssa" << "srt";
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
    //! @todo: check extension
    if (event->mimeData()->hasUrls())
    {
        QString path = UrlToPath(event->mimeData()->urls().first());
        if (!path.isEmpty()) {
            this->openSubtitles(path);
            event->acceptProposedAction();
        }
    }
}

//! @todo: merge
void MainWindow::on_btOpenSubtitles_clicked()
{
    QSettings settings;
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          "Выберите файл",
                                                          settings.value(DEFAULT_DIR_KEY).toString(),
                                                          FILETYPES_FILTER);

    if (fileName.isEmpty()) return;
    settings.setValue(DEFAULT_DIR_KEY, QFileInfo(fileName).absoluteDir().path());

    this->openSubtitles(fileName);
}

void MainWindow::on_btOpenCSV_clicked()
{
    QSettings settings;
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          "Выберите файл",
                                                          settings.value(DEFAULT_DIR_KEY).toString(),
                                                          "CSV (*.csv)");

    if (fileName.isEmpty()) return;
    settings.setValue(DEFAULT_DIR_KEY, QFileInfo(fileName).absoluteDir().path());

    this->openCSV(fileName);
}

void MainWindow::on_btSaveCSV_clicked()
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


//
// Функции
//
QString UrlToPath(const QUrl &url)
{
    QString path = url.toLocalFile();
    if (!path.isEmpty() && FILETYPES.contains(QFileInfo(path).suffix(), Qt::CaseInsensitive)) return path;

    return QString::null;
}

//! @todo: merge
void MainWindow::openSubtitles(const QString &fileName)
{
    // Очистка
    ui->btSaveCSV->setEnabled(false);
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
    else if (ui->edPrefix->text().isEmpty())
    {
        QMessageBox::warning(this, "Предупреждение", "Возможно, вы забыли ввести префикс кода.");
    }

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

    const QString prefix = ui->edPrefix->text();
    const QRegExp endLineTag("\\\\n", Qt::CaseInsensitive), assTags("\\{[^\\}]*\\}", Qt::CaseInsensitive);
    QList<QStandardItem*> row;
    uint pos = 1;
    foreach (const Script::Line::Event* event, script.events.content)
    {
        row.append(new QStandardItem( QString("%1-%2").arg(prefix).arg(pos) ));
        row.append(new QStandardItem( Script::Line::TimeToStr(event->start, Script::SCR_ASS) ));
        row.append(new QStandardItem( event->style.trimmed() ));
        row.append(new QStandardItem( event->text.trimmed().replace(endLineTag, " ").replace(assTags, QString::null) ));

        this->data.appendRow(row);
        row.clear();

        ++pos;
    }

    //! @todo: Определение единых фраз
    /*const QRegExp phraseNotBegin("^\\W*[a-zа-яё]"), phraseNotEnd("[^.?!…]$");
    for (int i = csv.length() - 1; i >= 0; --i)
    {
        if (i > 0)
        {
            const CSVLine& prev = csv.at(i - 1);
            const CSVLine& cur = csv.at(i);
            if ( cur.style == prev.style && ( cur.text.contains(phraseNotBegin) || prev.text.contains(phraseNotEnd) ) )
            {
                csv[i - 1].text = prev.text + " " + cur.text;
                csv.removeAt(i);
            }
        }
    }*/

    this->updateStyles();

    ui->btSaveCSV->setEnabled(true);
}

void MainWindow::openCSV(const QString &fileName)
{
    // Очистка
    ui->btSaveCSV->setEnabled(false);
    this->fileInfo.setFile(fileName);
    this->data.clear();

    // Чтение файла
    CSVReader reader;
    if (!reader.read(fileName, &this->data))
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка открытия файла");
        return;
    }

    if (this->data.columnCount() != 4)
    {
        QMessageBox::critical(this, "Ошибка", "Файл не соответствует принятому формату");
        return;
    }

    this->updateStyles();

    ui->btSaveCSV->setEnabled(true);
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
