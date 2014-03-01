#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QStandardItemModel>


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
    void on_btRenumber_clicked();

private:
    Ui::MainWindow *ui;
    QFileInfo fileInfo;
    QStandardItemModel data;
    QStringList checkedStyles;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void openFile(const QString &fileName);
    bool openSubtitles(const QString &fileName);
    bool openCSV(const QString &fileName);
    void saveCSV(const QString &fileName);
    void updateStyles();
    void renumber();
};

#endif // MAINWINDOW_H
