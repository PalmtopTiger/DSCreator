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
    void on_btOpenSubtitles_clicked();
    void on_btOpenCSV_clicked();
    void on_btSaveCSV_clicked();
    void on_lstStyles_itemChanged(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
    QFileInfo fileInfo;
    QStandardItemModel data;
    QStringList checkedStyles;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void openSubtitles(const QString &fileName);
    void openCSV(const QString &fileName);
    void saveCSV(const QString &fileName);
    void updateStyles();
};

#endif // MAINWINDOW_H
