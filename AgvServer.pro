QT += core sql network
QT -= gui

CONFIG += c++11

TARGET = AgvServer
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    util/common.cpp \
    util/global.cpp \
    sql/sql.cpp \
    sql/sqlserver.cpp \
    business/agvstation.cpp \
    business/agvline.cpp \
    business/agv.cpp \
    business/agvcenter.cpp \
    business/mapcenter.cpp \
    business/taskcenter.cpp \
    business/msgcenter.cpp \
    business/usermsgprocessor.cpp \
    business/agvpositionpublisher.cpp \
    business/agvstatuspublisher.cpp \
    log/agvlog.cpp \
    log/agvlogprocess.cpp \
    business/bezierarc.cpp \
    business/agvtaskpublisher.cpp \
    service/taskmaker.cpp \
    service/taskmakerworker.cpp \
    network/qyhzmqserver.cpp \
    network/qyhzmqserverworker.cpp \
    network/qyhzmqftp.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    util/common.h \
    util/concurrentqueue.h \
    util/global.h \
    sql/sql.h \
    sql/sqlserver.h \
    business/agvstation.h \
    business/agvline.h \
    business/agv.h \
    business/agvcenter.h \
    business/mapcenter.h \
    business/taskcenter.h \
    business/msgcenter.h \
    business/usermsgprocessor.h \
    business/agvpositionpublisher.h \
    business/agvstatuspublisher.h \
    log/agvlog.h \
    log/agvlogprocess.h \
    business/bezierarc.h \
    business/agvtaskpublisher.h \
    business/agvtask.h \
    service/taskmaker.h \
    service/taskmakerworker.h \
    network/qyhzmqserver.h \
    network/qyhzmqserverworker.h \
    network/qyhzmqftp.h

#pluginXml
INCLUDEPATH += D:\thirdparty\pugixml\include
CONFIG(debug, debug|release) {
    LIBS += D:\thirdparty\pugixml\staticlib\Debug\pugixml.lib
} else {
    LIBS += D:\thirdparty\pugixml\staticlib\Release\pugixml.lib
}

#QyhTcpLib
INCLUDEPATH += D:\thirdparty\QyhTcpLib\include
CONFIG(debug, debug|release) {
    LIBS += D:\thirdparty\QyhTcpLib\lib\Debug\QyhTcpLib.lib
} else {
    LIBS += D:\thirdparty\QyhTcpLib\lib\Release\QyhTcpLib.lib
}

#zeromq
INCLUDEPATH += D:\thirdparty\zeromq\include
CONFIG(debug, debug|release) {
    LIBS += D:\thirdparty\zeromq\lib\debug\dynamic\libzmq.lib
} else {
    LIBS += D:\thirdparty\zeromq\lib\release\dynamic\libzmq.lib
}

