#include "qualitydlg.h"
#include <QVBoxLayout>
#include <QRadioButton>
#include <QLabel>

QualityDlg::QualityDlg(hls_media_playlist_t* me, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Quality Choose");
    QLabel *lb = new QLabel("Please choose one quality:");
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(lb);
    int i = 1;
    mChooseQualityIdx = 1;
    while (me) {
        QString str = QString("%1: Bandwidth: %2,\t Resolution: %3,\t Codecs: %4").arg(i).arg(me->bitrate).arg(me->resolution).arg(me->codecs);
        QRadioButton* rb = new QRadioButton(str);
        rbVector.append(rb);
        layout->addWidget(rb);
        i += 1;
        me = me->next;
    }
    rbVector.at(0)->setChecked(true);
    okBtn = new QPushButton("OK");
    layout->addWidget(okBtn, Qt::AlignRight);
    setLayout(layout);
    connect(okBtn, SIGNAL(clicked(bool)), SLOT(chooseQuality()));
}

QualityDlg::~QualityDlg()
{
}

int QualityDlg::getChosedQualityIdx()
{
    return mChooseQualityIdx;
}

void QualityDlg::chooseQuality()
{
    foreach (QRadioButton* rb, rbVector) {
        if(rb->isChecked())
        {
            mChooseQualityIdx = rbVector.indexOf(rb) + 1;
            break;
        }
    }
    hide();
}
