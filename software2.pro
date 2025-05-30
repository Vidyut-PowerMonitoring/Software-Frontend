QT       += core gui charts websockets printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    LocationDetailDialog.cpp \
    ModernGaugeWidget.cpp \
    ScheduleManagerDialog.cpp \
    datarecorddialog.cpp \
    main.cpp \
    mainwindow.cpp\

HEADERS += \
    LocationDetailDialog.h \
    ModernGaugeWidget.h \
    ScheduleManagerDialog.h \
    datarecorddialog.h \
    mainwindow.h\



FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
