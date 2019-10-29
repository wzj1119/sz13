QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    log.cpp \
    main.cpp \
    mongodb.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    mongodb.h \
    log.h

LIBS +=/usr/local/lib/libevent.so

INCLUDEPATH += /usr/include/libbson-1.0/
INCLUDEPATH += /usr/include/libmongoc-1.0/
LIBS += /usr/lib/x86_64-linux-gnu/libmongoc-1.0.so
LIBS += /usr/lib/x86_64-linux-gnu/libbson-1.0.so
