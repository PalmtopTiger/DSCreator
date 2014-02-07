#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopWidget>
#include <QTextCodec>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>

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

void MainWindow::openFile(const QString &fileName)
{
    // Очистка
    ui->btSave->setEnabled(false);
    ui->lstStyles->clear();
    this->script.clear();

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
        if ( !Script::ParseSSA(in, this->script) )
        {
            QMessageBox::critical(this, "Ошибка", "Файл не соответствует формату SSA/ASS");
            fin.close();
            return;
        }
        break;

    case Script::SCR_SRT:
        if ( !Script::ParseSRT(in, this->script) )
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

    // Заполнение
    QStringList names;
    foreach (const Script::Line::Style* style, this->script.styles.content)
    {
        names.append(style->styleName);
    }
    names.sort();
    foreach (const QString& name, names)
    {
        QListWidgetItem* item = new QListWidgetItem(name, ui->lstStyles);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }

    ui->btSave->setEnabled(true);
}

void MainWindow::saveFile(const QString &fileName)
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

    if (ui->chkHeader->isChecked()) out << QString("Номер;Время;Стиль;Текст\n");

    const QString prefix = ui->edPrefix->text();
    QString text;
    uint pos = 1;
    foreach (const Script::Line::Event* event, this->script.events.content)
    {
        if (this->checkedStyles.isEmpty() || this->checkedStyles.contains(event->style, Qt::CaseInsensitive))
        {
            text = event->text;
            //! @todo: format
            out << QString("%1-%2;%3;%4;\"%5\"\n")
                   .arg(prefix)
                   .arg(pos) // .arg(pos, 3, 10, QChar('0'))
                   .arg(Script::Line::TimeToStr(event->start, Script::SCR_ASS))
                   .arg(event->style)
                   .arg(text.replace(QRegExp("\\\\n", Qt::CaseInsensitive), " ").replace(QChar('"'), "\"\""));
        }
        ++pos;
    }

    fout.close();
}

//
// Слоты
//
void MainWindow::on_btOpen_clicked()
{
    QSettings settings;
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          "Выберите файл",
                                                          settings.value(DEFAULT_DIR_KEY).toString(),
                                                          FILETYPES_FILTER);

    if (fileName.isEmpty()) return;

    this->fileInfo.setFile(fileName);
    settings.setValue(DEFAULT_DIR_KEY, this->fileInfo.absolutePath());

    this->openFile(this->fileInfo.fileName());
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

    this->saveFile(fileName);
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
