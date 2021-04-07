#ifndef DOWNLOADTHREAD_H
#define DOWNLOADTHREAD_H
#include <QThread>
#include "core/hls.h"
#include <QSemaphore>

class DownloadThread : public QThread
{
    Q_OBJECT
public:
    DownloadThread(QString file);
    ~DownloadThread();
    void run();
    typedef enum MSG_LEVEL{
        INFO_MSG,
        WARNING_MSG,
        ERROR_MSG,
    }MsgLevel;

    void setHadChosedQuality(int index);
signals:
    void sendMsg(QString msg, int level=INFO_MSG);

    void sendMediaPlayList(void* list);


private:
    QString mFileName;

    int mQualityIndex;

    QSemaphore *mQualitySem;

    hls_media_playlist_t media_playlist;

    hls_media_playlist_t audio_media_playlist;

    FILE *pFile;

private:
    bool getDataWithRetry(char *url, char **hlsfile_source, char **finall_url, int tries);

    int selectMediaPlaylist(hls_media_playlist_t *media_playlist, hls_media_playlist_t *audio_media_playlist, char *url, char *hlsfile_source, int playlist_type);

    int saveMedia(hls_media_playlist_t* media_playlist, hls_media_playlist_t* audio_media_playlist);

};

#endif // DOWNLOADTHREAD_H
