#include <QtGui/QApplication>
#include "updater.h"
#include <iostream>
#include "logger.h"
#include <QFile>
#include <QDir>
#include <QSettings>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    if(argc < 2) {
        std::cout << "Usage:\n"
                << "  updater <settings_ini_file>\n----\n"
                << "File format:\n[Updater]\n"
                << "  program_name=<your program name>\n"
                << "  local_path=<path to store files>\n"
                << "  URL=<URL to update.xml file>\n";

        return 1;
    }

    // Log file preparation
    QFile *logFile = new QFile(QDir::homePath() + "/.updater/updater.log");
    logFile->open(QIODevice::Append | QIODevice::Text);
    SLogger *logger = SLogger::instance();
    logger->setDevice(logFile);
    logger->log() << QString(" - - - - - - - - - Start logging -- ")
            << " " << QDate::currentDate().toString("dd.MM.yyyy")
            << " " << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;

    QSettings settings(argv[1], QSettings::IniFormat);
    settings.beginGroup("Updater");
    QString pn, lp, url;
    pn = settings.value("program_name").toString();
    lp = settings.value("local_path").toString();
    url = settings.value("URL").toString();
    settings.endGroup();

    if(pn.isEmpty() || lp.isEmpty() || url.isEmpty()) {
        std::cout << "Config file error!" << std::endl;
        return 1;
    }

    Updater w(pn, lp, url);
    w.checkForUpdates();
    w.show();
    return a.exec();
}
