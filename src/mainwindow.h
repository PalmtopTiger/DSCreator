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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btOpen_clicked();
    void on_btSave_clicked();
    void on_lstStyles_itemChanged(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
    QSettings _settings;
    QFileInfo _fileInfo;
    Script::Script _script;
    QStringList _checkedStyles;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void openSubtitles(const QString &fileName);
    void saveCSV(const QString &fileName);
    void updateStyles();
};

#endif // MAINWINDOW_H
