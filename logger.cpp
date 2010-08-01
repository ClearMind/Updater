/*! \file logger.cpp
    \brief Реализация класса описывающего пользователя.
*/

#include "logger.h"

SLogger *SLogger::inst = 0;

void SLogger::setDevice(QIODevice *d) {
    stream.setDevice(d);
}

QTextStream &SLogger::log() {
    return stream;
}
