#include "filetransferview.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QtCore>
#include "xmodem.h"

// ���캯��
FileTransferView::FileTransferView(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    connect(&thread, &FileThread::sendData, this, &FileTransferView::portSendData);
    connect(ui.stopButton, SIGNAL(clicked()), this, SLOT(cancelTransfer()));
    connect(ui.browseButton, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));
    connect(ui.startButton, SIGNAL(clicked()), this, SLOT(sendFile()));
    connect(&thread, SIGNAL(transFinsh()), this, SLOT(onTransFinsh()));
}

void FileTransferView::retranslate()
{
    ui.retranslateUi(this);
}

/* ��ȡ�����ļ� */
void FileTransferView::loadConfig(QSettings *config)
{
    config->beginGroup("FileTransmit");
    ui.beforeSendEdit->setText(config->value("BeforeSendText").toString());
    ui.enableBerforSendBox->setChecked(config->value("BeforeSend").toBool());
    ui.pathBox->setText(config->value("FileName").toString());
    ui.protocolBox->setCurrentText(config->value("Protocol").toString());
    if (config->value("SendMode").toBool()) {
        ui.sendButton->setChecked(true);
    } else {
        ui.receiveButton->setChecked(true);
    }
    config->endGroup();
}

// ��������
void FileTransferView::saveConfig(QSettings *config)
{
    config->beginGroup("FileTransmit");
    config->setValue("BeforeSendText", QVariant(ui.beforeSendEdit->toPlainText()));
    config->setValue("BeforeSend", QVariant(ui.enableBerforSendBox->isChecked()));
    config->setValue("FileName", QVariant(ui.pathBox->text()));
    config->setValue("Protocol", QVariant(ui.protocolBox->currentText()));
    config->setValue("SendMode", QVariant(ui.sendButton->isChecked()));
    config->endGroup();
}

// ���ļ���ť����
void FileTransferView::browseButtonClicked()
{
    QString fname;
    if (ui.sendButton->isChecked()) {
        fname = QFileDialog::getOpenFileName(this, "Open File", ui.pathBox->text());
    } else {
        fname = QFileDialog::getSaveFileName(this, "Open File", ui.pathBox->text());
    }
    if (!fname.isEmpty()) { // �ļ�����Ч
        ui.pathBox->setText(fname);
    }
}

// �����ļ�
void FileTransferView::sendFile()
{
    bool res = true;
    FileThread::TransMode mode[] {
        FileThread::SendMode,
        FileThread::ReceiveMode
    };
    QFile file(ui.pathBox->text());

    if (ui.sendButton->isChecked()) { // ����ģʽҪ���ļ�������ֻ����ʽ��
        res = file.open(QFile::ReadOnly);
        if (res) {
            file.close();
        }
    } else { // ����ģʽҪ���ļ�������ֻд��ʽ��
        res = file.open(QFile::WriteOnly);
        if (res) {
            file.close();
        }
    }
    if (res) {
        thread.setFileName(ui.pathBox->text());
        thread.setProtocol(FileThread::XModem);
        thread.setTransMode(mode[!ui.sendButton->isChecked()]);
        beforceSend(); // Ԥ�����ı�
        thread.startTransfer();
        ui.startButton->setEnabled(false);
        ui.stopButton->setEnabled(true);
        QString string(tr("Start transmit file: \"")
            + ui.pathBox->text() + "\".");
        logOut(string, Qt::blue);
    } else { // �޷����ļ�
        QString string(tr("Can not open the file: \"")
            + ui.pathBox->text() + "\".\n");

        logOut(tr("Error: ") + string, Qt::red);
        QMessageBox err(QMessageBox::Critical,
            tr("Error"),
            string,
            QMessageBox::Cancel, this);
        err.exec();
    }
}

// ��������
void FileTransferView::portSendData(const QByteArray &array)
{
    QString string;
    emit sendData(array);

    ui.progressBar->setValue(thread.progress());
}

// ��������
void FileTransferView::readData(const QByteArray &array)
{
    thread.readData(array);
}

// ȡ������
void FileTransferView::cancelTransfer()
{
    if (thread.cancelTransfer()) {
        logOut(tr("Cancel transfer.\n"), Qt::blue);
        ui.startButton->setEnabled(true);
        ui.stopButton->setEnabled(false);
    }
}

// ��ʾһ��log
void FileTransferView::logOut(const QString &string, QColor color)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");

    ui.textEdit->setTextColor(color);
    ui.textEdit->append("[" + time + "] " + string);
}

// ������ɲ�
void FileTransferView::onTransFinsh()
{
    ui.startButton->setEnabled(true);
    ui.stopButton->setEnabled(false);
    logOut(tr("Transmit finished.\n"), Qt::darkGreen);
}

// Ԥ�����ı�
void FileTransferView::beforceSend()
{
    // ֻ���ڷ���ģʽ�²ſ���ʹ��Ԥ����ģʽ
    if (ui.sendButton->isChecked() && ui.enableBerforSendBox->isChecked()) {
        QTextCodec *code = QTextCodec::codecForName("GB-2312");
        QByteArray arr = code->fromUnicode(ui.beforeSendEdit->toPlainText());

        emit sendData(arr);
    }
}
