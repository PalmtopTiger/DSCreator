#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "table.h"
#include <QMainWindow>
#include <QSettings>
#include <QFileInfo>
#include <QListWidgetItem>


namespace Ui {
class MainWindow;
}

enum Format {FMT_CSV, FMT_TSV};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btOpenSubtitles_clicked();
    void on_btSaveCSV_clicked();
    void on_btSaveTSV_clicked();
    void on_lstStyles_itemChanged(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
    QSettings _settings;
    QFileInfo _fileInfo;
    Table::Table _table;
    QStringList _checkedStyles;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void updateStyles();
    void open(const QString &fileName);
    void save(const QString &fileName, Format format);
};

#endif // MAINWINDOW_H
