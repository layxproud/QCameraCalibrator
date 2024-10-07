#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QShortcut>
#include <QUuid>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , workspace(new Workspace(this))
    , graphicsViewContainer(new GraphicsViewContainer(this))
    , configurationsWidget(new ConfigurationsWidget(this))
{
    ui->setupUi(this);
    resize(1180, 560);

    ui->cameraLayout->addWidget(graphicsViewContainer);
    ui->editorLayout->addWidget(configurationsWidget);
    ui->toolBox->setCurrentIndex(0);

    QShortcut *captuteFrameShortcut = new QShortcut(Qt::Key_Space, ui->captureButton);

    // Connects from GUI
    connect(ui->captureButton, &QPushButton::clicked, workspace, &Workspace::onCaptureFrame);
    QObject::connect(
        captuteFrameShortcut, &QShortcut::activated, workspace, &Workspace::onCaptureFrame);
    connect(ui->calibrateButton, &QPushButton::clicked, workspace, &Workspace::onStartCalibration);
    connect(ui->toolBox, &QToolBox::currentChanged, workspace, &Workspace::onPageChanged);
    connect(
        ui->markerSizeInput,
        QOverload<int>::of(&QSpinBox::valueChanged),
        workspace,
        &Workspace::onMarkerSizeChanged);
    connect(ui->saveConfigButton, &QPushButton::clicked, this, &MainWindow::onSaveConfiguration);
    connect(this, &MainWindow::pointSelected, workspace, &Workspace::pointSelected);
    connect(this, &MainWindow::saveConfiguration, workspace, &Workspace::saveConfiguration);
    connect(
        configurationsWidget,
        &ConfigurationsWidget::editConfiguration,
        workspace,
        &Workspace::saveConfiguration);
    connect(
        configurationsWidget,
        &ConfigurationsWidget::removeConfiguration,
        workspace,
        &Workspace::removeConfiguration);

    // Connects to GUI
    connect(
        workspace,
        &Workspace::frameReady,
        graphicsViewContainer,
        &GraphicsViewContainer::updateFrame);
    connect(workspace, &Workspace::taskFinished, this, &MainWindow::onTaskFinished);
    connect(
        workspace,
        &Workspace::calibrationParamsMissing,
        this,
        &MainWindow::onCalibrationParametersMissing);
    connect(workspace, &Workspace::newConfiguration, this, &MainWindow::onNewConfiguration);
    connect(workspace, &Workspace::configurationsUpdated, this, &MainWindow::onCofigurationsUpdated);
    connect(workspace, &Workspace::calibrationUpdated, this, &MainWindow::onCalibrationUpdated);
    connect(workspace, &Workspace::frameCaptured, this, &MainWindow::onFrameCaptured);

    workspace->init();
    configurationsWidget->setConfigurations(workspace->getConfigurations());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (graphicsViewContainer->geometry().contains(event->pos())) {
            QGraphicsView *view = graphicsViewContainer->getView();
            QPointF mappedPos = view->mapToScene(view->mapFromGlobal(event->globalPos()));
            emit pointSelected(mappedPos);
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::onTaskFinished(bool success, const QString &message)
{
    if (success) {
        QMessageBox::information(this, tr("Успех"), message);
    } else {
        QMessageBox::warning(this, tr("Ошибка"), message);
    }
}

void MainWindow::onNewConfiguration(const Configuration &config)
{
    ui->blockIdInput->setText(QString::fromStdString(config.id));
    ui->blockDateInput->setText(QString::fromStdString(config.date));
    ui->blockNameInput->setText(QString::fromStdString(config.name));
    ui->blockTypeInput->setText(QString::fromStdString(config.type));  
}

void MainWindow::onCalibrationParametersMissing()
{
    ui->toolBox->setCurrentIndex(0);
}

void MainWindow::onSaveConfiguration()
{
    Configuration newConfiguration{};
    newConfiguration.name = ui->blockNameInput->text().toStdString();
    newConfiguration.type = ui->blockTypeInput->text().toStdString();
    newConfiguration.date
        = QString(QDateTime::currentDateTime().toString("dd-MM-yyyy")).toStdString();
    if (ui->blockIdInput->text() == "") {
        QUuid uuid = QUuid::createUuid();
        QString uuidString = uuid.toString(QUuid::WithoutBraces);
        newConfiguration.id = uuidString.toStdString();
    } else {
        newConfiguration.id = ui->blockIdInput->text().toStdString();
    }
    emit saveConfiguration(newConfiguration, false);
}

void MainWindow::onCofigurationsUpdated()
{
    configurationsWidget->setConfigurations(workspace->getConfigurations());
}

void MainWindow::onCalibrationUpdated(bool status)
{
    if (status) {
        ui->calibrationStatusLabel->setText(tr("Файл калибровки загружен"));
    } else {
        ui->calibrationStatusLabel->setText(tr("Файл калибровки отсутствует"));
    }
}

void MainWindow::onFrameCaptured(int num)
{
    ui->framesCapturedValue->setText(QString::number(num));
}
