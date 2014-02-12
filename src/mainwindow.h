#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "script.h"
#include <QMainWindow>
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
    QFileInfo fileInfo;
    Script::Script script;
    QStringList checkedStyles;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void openFile(const QString &fileName);
    void saveFile(const QString &fileName);
};

#endif // MAINWINDOW_H
