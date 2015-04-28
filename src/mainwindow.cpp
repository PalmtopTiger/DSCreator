#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtMath>
#include <QTextCodec>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QMimeData>
#include <QUrl>
#include <QStandardItemModel>

QString UrlToPath(const QUrl &url);
QString TimeToPT(const uint time, const double fps);

const QString DEFAULT_DIR_KEY = "DefaultDir";
const QString PREFIX_KEY = "Prefix";
const QString FPS_KEY = "FPS";
const QStringList FILETYPES = QStringList() << "ass" << "ssa" << "srt";
#if QT_VERSION >= 0x050000
const QString FILETYPES_FILTER = "Субтитры (*." + FILETYPES.join(" *.") + ")";
#else
const QString FILETYPES_FILTER = QTextCodec::codecForName("UTF-8")->toUnicode("Субтитры") + " (*." + FILETYPES.join(" *.") + ")";
#endif
const QChar CSV_SEPARATOR = ';';
enum { COL_ID, COL_START, COL_END, COL_STYLE, COL_TEXT, COL_COUNT };


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->edPrefix->setText(_settings.value(PREFIX_KEY, ui->edPrefix->text()).toString());
    ui->edFPS->setValue(_settings.value(FPS_KEY, ui->edFPS->value()).toDouble());

    this->move(QApplication::desktop()->screenGeometry().center() - this->rect().center());
}

MainWindow::~MainWindow()
{
    _settings.setValue(PREFIX_KEY, ui->edPrefix->text());
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
            this->openSubtitles(fileName);
        }
    }
}

void MainWindow::on_btOpen_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          "Выберите файл",
                                                          _settings.value(DEFAULT_DIR_KEY).toString(),
                                                          FILETYPES_FILTER);

    if (fileName.isEmpty()) return;

    _settings.setValue(DEFAULT_DIR_KEY, QFileInfo(fileName).absoluteDir().path());

    this->openSubtitles(fileName);
}

void MainWindow::on_btSave_clicked()
{
    QString templateName = _fileInfo.path() + QDir::separator() + _fileInfo.baseName();
    if (!_checkedStyles.isEmpty()) templateName.append(" (" + _checkedStyles.join(",") + ')');
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

QString TimeToPT(const uint time, const double fps)
{
    uint hour = time / 3600000u,
            min  = time % 3600000u / 60000u,
            sec  = time % 60000u   / 1000u,
            msec = time % 1000u;

    return QString("%1:%2:%3:%4")
            .arg(hour, 2, 10, QChar('0'))
            .arg(min, 2, 10, QChar('0'))
            .arg(sec, 2, 10, QChar('0'))
            .arg(qFloor(msec * fps / 1000.0), 2, 10, QChar('0'));
}

void MainWindow::openSubtitles(const QString &fileName)
{
    // Очистка
    ui->btSave->setEnabled(false);
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

    this->updateStyles();
    ui->btSave->setEnabled(true);
}

void MainWindow::saveCSV(const QString &fileName)
{
    // Построение таблицы
    QStandardItemModel table;
    const QRegExp endLineTag("\\\\n", Qt::CaseInsensitive), assTags("\\{[^\\}]*\\}", Qt::CaseInsensitive);
    const QString prefix = ui->edPrefix->text();
    QList<QStandardItem*> row;
    uint pos = 1;
    QString id;
    foreach (const Script::Line::Event* event, _script.events.content)
    {
        id = QString::number(pos);
        if (!prefix.isEmpty()) {
            id.prepend('-');
            id.prepend(prefix);
        }

        row.append(new QStandardItem( id ));
        row.append(new QStandardItem( TimeToPT(event->start, ui->edFPS->value()) ));
        row.append(new QStandardItem( TimeToPT(event->end, ui->edFPS->value()) ));
        row.append(new QStandardItem( event->style.trimmed() ));
        row.append(new QStandardItem( event->text.trimmed().replace(endLineTag, " ").replace(assTags, QString::null) ));

        table.appendRow(row);
        row.clear();
        ++pos;
    }

    // Определение единых фраз
    const QRegExp phraseNotBegin("^\\W*[a-zа-яё]"), phraseNotEnd("[^.?!…]$");
    for (int row = table.rowCount() - 1; row > 0; --row)
    {
        const int prev_row = row - 1;
        // Если стиль совпадает, текст прошлой не оканчивается на точку и текущая начинается с маленькой буквы
        if ( table.item(row, COL_STYLE)->text() == table.item(prev_row, COL_STYLE)->text() &&
             table.item(row, COL_TEXT)->text().contains(phraseNotBegin) &&
             table.item(prev_row, COL_TEXT)->text().contains(phraseNotEnd) )
        {
            table.item(prev_row, COL_END)->setText( table.item(row, COL_END)->text() );
            table.item(prev_row, COL_TEXT)->setText( table.item(prev_row, COL_TEXT)->text() + " " + table.item(row, COL_TEXT)->text() );
            table.removeRow(row);
        }
    }

    QFile fout(fileName);
    if ( !fout.open(QFile::WriteOnly | QFile::Text) )
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка сохранения файла");
        return;
    }

    QTextStream out(&fout);
    out.setCodec( QTextCodec::codecForName("UTF-8") );
    out.setGenerateByteOrderMark(true);

    QString temp;
    QStringList tempLine;
    for (int row = 0; row < table.rowCount(); ++row)
    {
        // Проверка находится в этом цикле потому, что иначе при пропуске части стилей текст ошибочно складывается в одну строку
        if ( _checkedStyles.isEmpty() || _checkedStyles.contains(table.item(row, COL_STYLE)->text(), Qt::CaseInsensitive) )
        {
            for (int col = 0; col < table.columnCount(); ++col)
            {
                temp = table.item(row, col)->text();
                if ( temp.contains(CSV_SEPARATOR) )
                {
                    temp = QString("\"%1\"").arg( temp.replace(QChar('"'), "\"\"") );
                }
                tempLine.append(temp);
            }
            out << tempLine.join(CSV_SEPARATOR) << QString("\n");
            tempLine.clear();
        }
    }

    fout.close();
}

void MainWindow::updateStyles()
{
    ui->lstStyles->clear();

    QSet<QString> styles;
    foreach (const Script::Line::Event* event, _script.events.content)
    {
        styles.insert(event->style);
    }
    QStringList sortedStyles = styles.values();
    sortedStyles.sort();
    foreach (const QString& style, sortedStyles)
    {
        QListWidgetItem* item = new QListWidgetItem(style, ui->lstStyles);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}
