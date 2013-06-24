TARGET = QmlProfilerExtension
TEMPLATE = lib

PROVIDER = Digia
include(../../qtcreatorplugin.pri)
include(qmlprofilerextension_dependencies.pri)

DEFINES += QMLPROFILEREXTENSION_LIBRARY

# QmlProfilerExtension files

SOURCES += qmlprofilerextensionplugin.cpp \
        scenegraphtimelinemodel.cpp \
        pixmapcachemodel.cpp

HEADERS += qmlprofilerextensionplugin.h \
        qmlprofilerextension_global.h \
        qmlprofilerextensionconstants.h \
        scenegraphtimelinemodel.h \
        pixmapcachemodel.h

OTHER_FILES += \
    QmlProfilerExtension.json
