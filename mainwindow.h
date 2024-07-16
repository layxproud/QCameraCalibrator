#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "graphicsviewcontainer.h"
#include "workspace.h"
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

signals:
    void pointSelected(const QPointF &point);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    Ui::MainWindow *ui;
    Workspace *workspace;
    GraphicsViewContainer *graphicsViewContainer;

private slots:
    void onCalibrationFinished(bool success, const QString &message);
    void onNewConfiguration(const std::string &name);
};
#endif // MAINWINDOW_H
