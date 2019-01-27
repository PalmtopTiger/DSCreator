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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btOpenSubtitles_clicked();
    void on_btSaveCSV_clicked();
    void on_btSaveTSV_clicked();
    void on_btSavePDF_clicked();
    void on_lsActors_itemClicked(QListWidgetItem* item);

private:
    Ui::MainWindow *ui;
    QSettings _settings;
    QFileInfo _fileInfo;
    Script::Script _script;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void updateActors();
    QStringList getCheckedActors() const;
    QString getSaveFileName(const QStringList& actors, const QString& suffix);
    int getTimeStart() const;
    void openFile(const QString &fileName);
};

#endif // MAINWINDOW_H
