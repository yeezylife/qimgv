#ifndef PRINTDIALOG_H
#define PRINTDIALOG_H

#include <QDialog>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrinterInfo>
#include <QFileDialog>
#include <QPainter>
#include <QGraphicsColorizeEffect>
#include <QDebug>
#include "components/thumbnailer/thumbnailer.h"
#include "settings.h"

namespace Ui {
class PrintDialog;
}

class PrintDialog : public QDialog {
    Q_OBJECT

public:
    explicit PrintDialog(QWidget *parent = nullptr);
    ~PrintDialog();
    void setImage(std::shared_ptr<const QImage> _img);
    void setOutputPath(QString path);

private slots:
    void print();
    void exportPdf();
    void updatePreview();
    void setLandscape(bool mode);
    void onPrinterSelected(QString name);

private:
    void initializePrinters();
    void setupConnections();
    void saveSettings();
    QRectF getImagePrintRect(QPrinter *printer);
    QString pdfPathDialog();
    
    Ui::PrintDialog *ui;
    std::shared_ptr<const QImage> img = nullptr;
    QPrinter pdfPrinter, *printer = nullptr;
    bool printPdfDefault = false;
};

#endif // PRINTDIALOG_H
