#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "script.h"
#include <QMainWindow>
#include <QSettings>
#include <QFileInfo>
#include <QListWidgetItem>


namespace Ui {
class MainWindow;
}

// FIXME: Use Writer
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
    void on_btSavePDF_clicked();
    void on_lstActors_itemChanged(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
    QSettings _settings;
    QFileInfo _fileInfo;
    Script::Script _script;
    QStringList _checkedActors;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void updateActors();
    void open(const QString &fileName);
    void save(const QString &fileName, const Format format);
};

#endif // MAINWINDOW_H
