#ifndef QUALITYDLG_H
#define QUALITYDLG_H

#include <QDialog>
#include "core/hls.h"
#include <QRadioButton>
#include <QPushButton>

class QualityDlg : public QDialog
{
    Q_OBJECT
public:
    explicit QualityDlg(hls_media_playlist_t* me, QWidget *parent = 0);
    ~QualityDlg();
    int getChosedQualityIdx();
signals:

private slots:
    void chooseQuality();

private:
     int mChooseQualityIdx;
     QVector<QRadioButton*> rbVector;
     QPushButton *okBtn;
};

#endif // QUALITYDLG_H
