TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    main.c \
    mongodb.c
LIBS +=/usr/local/lib/libevent.so

INCLUDEPATH += /usr/include/libbson-1.0/
INCLUDEPATH += /usr/include/libmongoc-1.0/
LIBS += /usr/lib/x86_64-linux-gnu/libmongoc-1.0.so
LIBS += /usr/lib/x86_64-linux-gnu/libbson-1.0.so

HEADERS += \
    mongodb.h
