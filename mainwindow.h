#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "calibrationthread.h"
#include <camerathread.h>
#include <opencv2/opencv.hpp>
#include <QDir>
#include <QGraphicsScene>
#include <QMainWindow>
#include <QMouseEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    CameraThread *cameraThread;
    CalibrationThread *calibrationThread;

    int frameNumber;
    QString imagesDir;

    void startThread(QThread *thread);
    void stopThread(QThread *thread);
    void clearDirectory(const QString &path);
    void ensureDirectoryIsClean(const QString &path);

private slots:
    void onPageChanged(int page);
    void onCaptureFrame();
    void onStartCalibration();
    void onCalibrationFinished(bool success);
    void updateFrame(const cv::Mat &frame);
};
#endif // MAINWINDOW_H
