#/***************************************************************************
# *   Copyright (C) 2013 by Torsten Flammiger                               *
# *   github@netfg.net                                                      *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License as published by  *
# *   the Free Software Foundation; either version 2 of the License, or     *
# *   (at your option) any later version.                                   *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU General Public License     *
# *   along with this program; if not, write to the                         *
# *   Free Software Foundation, Inc.,                                       *
# *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
# ***************************************************************************/

QT       += core
QT       -= gui

TARGET = muttvcardsearch
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app
target.path = /usr/bin
INSTALLS += target

SOURCES += main.cpp \
    cardcurler.cpp \
    person.cpp \
    settings.cpp \
    option.cpp

LIBS += -lcurl -lvcard

HEADERS += \
    cardcurler.h \
    person.h \
    settings.h \
    option.h \
    version.h

RESOURCES += \
    carddavquery.qrc

