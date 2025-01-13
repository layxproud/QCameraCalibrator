#include "configurationswidget.h"
#include "section.h"
#include <QDateTime>

ConfigurationForm::ConfigurationForm(QWidget *parent)
    : QWidget(parent)
{
    blockIdLabel = new QLabel(tr("Block ID"), this);
    blockNameLabel = new QLabel(tr("Block name"), this);
    blockTypeLabel = new QLabel(tr("Block type"), this);
    blockDateLabel = new QLabel(tr("Date of creation"), this);
    emptyLabel = new QLabel(this);

    blockIdInput = new QLineEdit(this);
    blockIdInput->setReadOnly(true);
    blockNameInput = new QLineEdit(this);
    blockTypeInput = new QLineEdit(this);
    blockDateInput = new QLineEdit(this);
    blockDateInput->setReadOnly(true);

    editButton = new QPushButton(tr("Edit"), this);
    connect(editButton, &QPushButton::clicked, this, &ConfigurationForm::onEditButton);
    deleteButton = new QPushButton(tr("Delete"), this);
    connect(deleteButton, &QPushButton::clicked, this, &ConfigurationForm::onDeleteButton);

    formLayout = new QFormLayout(this);
    formLayout->addRow(blockIdLabel, blockIdInput);
    formLayout->addRow(blockDateLabel, blockDateInput);
    formLayout->addRow(blockNameLabel, blockNameInput);
    formLayout->addRow(blockTypeLabel, blockTypeInput);
    formLayout->addRow(deleteButton, editButton);

    setLayout(formLayout);
}

void ConfigurationForm::setData(const Configuration &config)
{
    blockIdInput->setText(QString::fromStdString(config.id));
    blockTypeInput->setText(QString::fromStdString(config.type));
    blockNameInput->setText(QString::fromStdString(config.name));
    blockDateInput->setText(QString::fromStdString(config.date));
    markerIds = config.markerIds;
    relativePoints = config.relativePoints;
}

Configuration ConfigurationForm::getData()
{
    Configuration config{};
    config.id = blockIdInput->text().toStdString();
    config.name = blockNameInput->text().toStdString();
    config.type = blockTypeInput->text().toStdString();
    config.date = QString(QDateTime::currentDateTime().toString("dd-MM-yyyy")).toStdString();
    config.markerIds = markerIds;
    config.relativePoints = relativePoints;
    return config;
}

void ConfigurationForm::onEditButton()
{
    emit editConfiguration(getData());
}

void ConfigurationForm::onDeleteButton()
{
    emit removeConfiguration(getData());
}

ConfigurationsWidget::ConfigurationsWidget(QWidget *parent)
    : QWidget(parent)
{}

void ConfigurationsWidget::setConfigurations(
    const std::map<std::string, Configuration> &configurations)
{
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());

    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
    } else {
        clearLayout(mainLayout);
    }

    if (configurations.empty()) {
        QLabel *messageLabel = new QLabel(tr("Configuration file is empty or missing"));
        mainLayout->addWidget(messageLabel);
    } else {
        for (const auto &pair : configurations) {
            Section *section = new Section(pair.second.name.c_str(), 300, this);
            QVBoxLayout *contentLayout = new QVBoxLayout();
            ConfigurationForm *widget = new ConfigurationForm();
            widget->setData(pair.second);
            connect(
                widget,
                &ConfigurationForm::editConfiguration,
                this,
                &ConfigurationsWidget::editConfiguration);
            connect(
                widget,
                &ConfigurationForm::removeConfiguration,
                this,
                &ConfigurationsWidget::removeConfiguration);
            contentLayout->addWidget(widget);
            section->setContentLayout(*contentLayout);
            mainLayout->addWidget(section);
        }
        mainLayout->addStretch(1);
    }
    if (!this->layout()) {
        this->setLayout(mainLayout);
    }
}

void ConfigurationsWidget::clearLayout(QLayout *layout)
{
    Q_ASSERT(layout != nullptr);

    while (QLayoutItem *item = layout->takeAt(0)) {
        QWidget *widget = item->widget();

        if (widget) {
            delete widget;
        } else {
            delete item;
        }
    }
}
