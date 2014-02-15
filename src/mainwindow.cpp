#include "mainwindow.h"
#include "ui_mainwindow.h"
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

typedef QPair<QString, uint> CodePair;

class CSVLine
{
public:
    CodePair code;
    uint time;
    QString style;
    QString text;

    CSVLine(const CodePair& code,
            const uint time,
            const QString& style,
            const QString& text) :
        code(code),
        time(time),
        style(style),
        text(text)
    {}

    QString join(const QString& separator = ";") const
    {
        return ( QStringList()
                 << QString("%1-%2").arg(code.first).arg(code.second)
                 << Script::Line::TimeToStr(time, Script::SCR_ASS)
                 << style
                 << QString("\"%1\"").arg( QString(text).replace(QChar('"'), "\"\"") ) ).join(separator);
    }
};


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
        QString path = UrlToPath(event->mimeData()->urls().first());
        if (!path.isEmpty()) {
            this->openFile(path);
            event->acceptProposedAction();
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
    settings.setValue(DEFAULT_DIR_KEY, fileName);

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
    this->fileInfo.setFile(fileName);

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

    QList<CSVLine> csv;
    const QString prefix = ui->edPrefix->text();
    uint pos = 1;
    foreach (const Script::Line::Event* event, this->script.events.content)
    {
        if (this->checkedStyles.isEmpty() || this->checkedStyles.contains(event->style, Qt::CaseInsensitive))
        {
            csv.append( CSVLine( CodePair(prefix, pos),
                                 event->start,
                                 event->style.trimmed(),
                                 event->text.trimmed().replace(QRegExp("\\\\n", Qt::CaseInsensitive), " ").replace(QRegExp("\\{[^\\}]*\\}", Qt::CaseInsensitive), QString::null) ) );
        }
        ++pos;
    }

    // Определение единых фраз
    for (int i = csv.length() - 1; i >= 0; --i)
    {
        if (i > 0)
        {
            const CSVLine& prev = csv.at(i - 1);
            const CSVLine& cur = csv.at(i);
            if ( cur.style == prev.style && cur.text.contains(QRegExp("^\\W*[a-zа-яё]")) ) // prev.text.contains(QRegExp("[.!?]$"))
            {
                csv[i - 1].text = prev.text + " " + cur.text;
                csv.removeAt(i);
            }
        }
    }

    QTextStream out(&fout);
    out.setCodec( QTextCodec::codecForName("UTF-8") );
    out.setGenerateByteOrderMark(true);
    if (ui->chkHeader->isChecked()) out << QString("Код;Время;Стиль;Текст\n");
    foreach (const CSVLine& line, csv) out << line.join() << "\n";
    fout.close();
}
