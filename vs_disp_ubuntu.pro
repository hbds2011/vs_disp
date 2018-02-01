TEMPLATE  = app
HEADERS   = \
    vs_disp_menu.h \
    verup_menu_api.h
SOURCES   = \
    vs_disp_menu.cpp \
    vs_disp_main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qws/vs_disp_ubuntu
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qws/vs_disp_ubuntu
INSTALLS += target sources

RESOURCES += \
    vs_disp.qrc

