#include "updater.h"
#include <QDate>
#include <QTime>
#include <QMap>

#define PLATFORM "linux"

Updater::Updater(const QString &pn, const QString &p, const QString &u, QWidget *parent):
        QWidget(parent),
        programName(pn),
        path(p),
        url(u),
        currentUrlIdx(0) {
    netReply = 0;
    proc = new QProcess(this);
    netManager = new QNetworkAccessManager(this);
    netRequest.setUrl(url);
    url.remove(url.lastIndexOf("/"), url.size());

    reader = new QXmlStreamReader();

    logger = SLogger::instance();

    // +++++ WIDGET +++++++++++++++++++++++++++++++++++++++++++++
    setWindowTitle(tr("Updater"));
    setFixedSize(500, 100);

    message = new QLabel(tr("Checking for updates..."));
    currentAction = new QLabel(tr("Reading file %1").arg(url));
    progressBar = new QProgressBar();
    progressBar->setValue(0);

    lay = new QVBoxLayout();
    lay->addWidget(message);
    lay->addWidget(currentAction);
    lay->addWidget(progressBar);

    setLayout(lay);

    setWindowIcon(QIcon(":/icon.png"));

    // create directories ++++++++++++++++++++++++++++++++++++++++++++++++++
    QDir dir(QDir::homePath() + "/.updater/" + programName);
    if(!dir.exists()) {
        // logging
        logger->log() << "Created standard directories: "
                << QDir::homePath() + "/.updater; "
                << QDir::homePath() + "/.updater/" + programName << endl;
        // mkdir
        dir.mkdir(QDir::homePath() + "/.updater");
        dir.mkdir(QDir::homePath() + "/.updater/" + programName);
    }
    connect(this, SIGNAL(updateFinished()), this, SLOT(run()));
    connect(proc, SIGNAL(finished(int)), this, SLOT(close()));
}

void Updater::checkForUpdates() {
    netReplyXml = netManager->get(netRequest);
    if(!netReplyXml) {
        logger->log() << QString("Error! -- ") + netReplyXml->errorString() << endl;
        emit updateFinished(); // run without update
//        exit(1);
    }

    // connection singnals and slots
    connect(netReplyXml, SIGNAL(readyRead()), this, SLOT(getXml()));
    connect(netReplyXml, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(setProgress(qint64, qint64)));
}

void Updater::getXml() {
    QFile fout(QDir::homePath() + "/.updater/" + programName + "/update.xml");
    if(!fout.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // error messages to log
        logger->log() << fout.errorString() << endl;
        emit updateFinished(); // run program without update
//        exit(2);
    }
    netReplyXml->setTextModeEnabled(true);
    QTextStream out(&fout);

    while(!netReplyXml->atEnd()) {
        QString buffer;
        buffer = netReplyXml->readLine(256);
        out << buffer;
    }
    logger->log() << "File saved: " << fout.fileName() << endl;
    fout.close();

    currentAction->setText(tr("File: update.xml cashed."));
    parseXml();
    processUpdates();
}

void Updater::parseXml() {
    QFile current(QDir::homePath() + "/.updater/" + programName + "/current.xml");
    if(current.exists()) {
        if(!current.open(QIODevice::ReadOnly | QIODevice::Text)) {
            logger->log() << current.errorString() << endl;
            emit updateFinished(); // run program without update
//            exit(3);
        }
    }
    QFile update(QDir::homePath() + "/.updater/" + programName + "/update.xml");
    if(!update.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logger->log() << update.errorString() << endl;
        emit updateFinished(); // run program without update
//        exit(4);
    }
    QMap<QString, QString> hashes;

    if(current.exists()) {
        reader->setDevice(&current);
        if(reader->readNextStartElement()) {
            if(reader->name() == "update" && reader->attributes().value("platform") == PLATFORM) {
                while (reader->readNextStartElement()) {
                    if (reader->name() == "file") {
                        hashes[reader->attributes().value("name").toString()] =
                                reader->attributes().value("hash").toString();
                        reader->skipCurrentElement();
                    }
                    else reader->skipCurrentElement();
                }
            } else reader->skipCurrentElement();
        }
    }

    reader->clear();
    reader->setDevice(&update);
    if(reader->readNextStartElement()) {
        if(reader->name() == "update" && reader->attributes().value("platform") == PLATFORM) {
            exe = reader->attributes().value("run").toString();
            while (reader->readNextStartElement()) {
                if (reader->name() == "file") {
                    if(hashes[reader->attributes().value("name").toString()] !=
                       reader->attributes().value("hash").toString()) {
                        // logging
                        logger->log() << "New updates available for file: " << reader->attributes().value("name").toString()
                                << " " << hashes[reader->attributes().value("name").toString()] << " --> " <<
                                reader->attributes().value("hash").toString() << endl;
                        // set message
                        message->setText("Updates available.");

                        updates << url + "/" + reader->attributes().value("name").toString();
                    }
                    reader->skipCurrentElement();
                } else reader->skipCurrentElement();
            }
        } else reader->skipCurrentElement();
    }
    reader->clear();
    current.close();
    update.close();
    progressBar->setValue(5);
    currentAction->setText("Process data...");
}

void Updater::processUpdates() {
    if(updates.size() == 0) {
        logger->log() << "No new updates available" << endl;
        message->setText(tr("No new updates."));
        currentAction->setText(tr("Running program..."));
        progressBar->setValue(100);
        emit updateFinished();
    } else {
        logger->log() << "    Downloading file: " << updates.at(currentUrlIdx) << endl;
        currentAction->setText(tr("Download file: %1").arg(updates.at(currentUrlIdx)));

        netRequest.setUrl(updates.at(currentUrlIdx));
        netReply = netManager->get(netRequest);
        if(!netReply) {
            logger->log() << netReply->errorString() << endl;
            exit(9);
        }
        connect(netReply, SIGNAL(finished()), this, SLOT(down()));
        connect(netReply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(setProgress(qint64, qint64)));
    }
}

void Updater::down() {
    QString filename;
    filename = netReply->url().toString().remove(url);

    QDir dir(path);
    if(!dir.exists()) {
        logger->log() << "Creating path: " << path << endl;
        dir.mkpath(path);
    }
    if(filename.lastIndexOf("/") != -1) {
        QDir d(path + filename.left(filename.lastIndexOf("/")));
        if(!d.exists()) {
            d.mkpath(path + filename.left(filename.lastIndexOf("/")));
        }
    }
    QFile out(path + filename);
    if(exe == filename.right(filename.size()-1)) {
        out.setPermissions(QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);
    }
    if(!out.open(QIODevice::WriteOnly)) {
        logger->log() << out.errorString() << endl;
        exit(9);
    }
    if(netReply->bytesAvailable() >0) {
        if(-1 == out.write(netReply->readAll())) {
            logger->log() << out.errorString() << endl;
            logger->log() << netReply->errorString() << endl;
            exit(17);
        } else {
            qDebug() << "Saved: " << out.fileName();
            currentAction->setText("Complete");
            currentUrlIdx++;
            if(currentUrlIdx < updates.size()) {
                disconnect(netReply, 0, this, 0);
                netRequest.setUrl(updates.at(currentUrlIdx));
                netReply = netManager->get(netRequest);
                if(!netReply) {
                    logger->log() << netReply->errorString() << endl;
                    exit(9);
                }
                logger->log() << "    Downloading file: " << updates.at(currentUrlIdx) << endl;
                connect(netReply, SIGNAL(finished()), this, SLOT(down()));
                connect(netReply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(setProgress(qint64, qint64)));
            } else {
                QProcess rmcp;
                rmcp.start("rm " + QDir::homePath() + "/.updater/" + programName + "/current.xml");
                rmcp.waitForFinished();
                rmcp.start("cp " + QDir::homePath() + "/.updater/" + programName + "/update.xml " +
                           QDir::homePath() + "/.updater/" + programName + "/current.xml");
                rmcp.waitForFinished();
                emit updateFinished();
            }
        }
    }
    out.close();
}

void Updater::run() {
    if(!exe.isEmpty()) {
        logger->log() << "Running program: " << exe << endl;

        proc->start(path + "/" + exe);
        this->hide();
    } else {
        this->close();
    }
}

void Updater::setProgress(qint64 a, qint64 t) {
    int current;
    if(t == 0)
        current = 0;
    else
        current = a*100/t;
    progressBar->setValue(current);
}

Updater::~Updater() {
    delete message;
    delete currentAction;
    delete progressBar;
    delete lay;

    delete netManager;
    delete reader;
    delete proc;
}
