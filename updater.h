#ifndef UPDATER_H
#define UPDATER_H

#include <QtGui>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QtXml>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QByteArray>
#include <QProcess>
#include "logger.h"

class Updater : public QWidget {
    Q_OBJECT

public:
    Updater(const QString &, const QString &, const QString &, QWidget *parent = 0);
    ~Updater();
    void checkForUpdates();
    void parseXml();
    void processUpdates();

signals:
    void updateFinished();

protected slots:
    void getXml();
    void setProgress(qint64, qint64);
    void down();
    void run();

private:
    // network
    QNetworkAccessManager *netManager;
    QNetworkReply *netReplyXml;
    QNetworkReply *netReply;
    QNetworkRequest netRequest;

    // xml
    QXmlStreamReader *reader;

    // widget elements
    QLabel *message;
    QLabel *currentAction;
    QProgressBar *progressBar;
    QVBoxLayout *lay;

    // parameters
    QSettings *settings;
    QString programName;
    QString path;
    QString url;

    // log
    SLogger *logger;

    QStringList updates;
    int currentUrlIdx;

    QString exe;

    QProcess *proc;
};

#endif // UPDATER_H
