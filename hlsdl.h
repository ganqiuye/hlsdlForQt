#ifndef HLSDL_H
#define HLSDL_H

#include <QWidget>
#include "core/hls.h"

namespace Ui {
class Hlsdl;
}

class Hlsdl : public QWidget
{
    Q_OBJECT

public:
    explicit Hlsdl(QWidget *parent = 0);
    ~Hlsdl();

private slots:
    void on_downloadBtn_clicked();

    void on_fileSaveBtn_clicked();

    void on_moreBtn_clicked();

private:
    Ui::Hlsdl *ui;

    void initArg();

    bool checkArg();

    bool getDataWithRetry(char *url, char **hlsfile_source, char **finall_url, int tries);

    int selectMediaPlaylist(hls_media_playlist_t *media_playlist, hls_media_playlist_t *audio_media_playlist, char *url, char *hlsfile_source, int playlist_type);

    int saveMedia(hls_media_playlist_t media_playlist, hls_media_playlist_t audio_media_playlist);

    int dowloading();

    FILE *getOutputFile();
};

#endif // HLSDL_H
