macx {
  # Silence warnings on Mac
  cache()
}

TEMPLATE = app

# Make sure we do not accidentally #include files placed in 'resources'
CONFIG += no_include_pwd
# Reduce compile times and avoid configuration confusion by excluding Qt libs
CONFIG -= qt
# Do not create an app bundle when running on OS X
CONFIG -= app_bundle

SOURCES = $$files($$PWD/*.cpp)
SOURCES += $$files($$PWD/StanfordCPPLib/*.cpp)
HEADERS = $$files($$PWD/*.h)
HEADERS += $$files($$PWD/StanfordCPPLib/*.h)

QMAKE_CXXFLAGS += -std=c++11
QMAKE_MAC_SDK = macosx10.9

INCLUDEPATH += $$PWD/StanfordCPPLib/

# The rest of this file defines how to copy the resources folder
defineTest(copyToDestdir) {
    files = $$1

    for(FILE, files) {
        DDIR = $$OUT_PWD

        # Replace slashes in paths with backslashes for Windows
        win32:FILE ~= s,/,\\,g
        win32:DDIR ~= s,/,\\,g

        !win32 {
            copyResources.commands += cp -r '"'$$FILE'"' '"'$$DDIR'"' $$escape_expand(\\n\\t)
        }
        win32 {
            copyResources.commands += xcopy '"'$$FILE'"' '"'$$DDIR'"' /e /y $$escape_expand(\\n\\t)
        }
    }
    export(copyResources.commands)
}
!win32 {
    copyToDestdir($$files($$PWD/resources/*))
}
win32 {
    copyToDestdir($$PWD/resources)
}

copyResources.input = $$files($$PWD/resources/*)
OTHER_FILES = $$files(resources/*)
QMAKE_EXTRA_TARGETS += copyResources
POST_TARGETDEPS += copyResources
