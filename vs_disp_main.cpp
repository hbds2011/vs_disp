/**
 * main.cpp for G3NVR
 * Copyright(C) 2002-2017 MegaChips Co.,Ltd All right reserved.
 * author: "Cao Tianyang"<caoty@hyron-js.com>
 */

#include <QApplication>
#include <QTextCodec>
#include <QTranslator>
#include "vs_disp_menu.h"

//! [create application]
int main(int argc, char *argv[])
{
	// 英語→日本変換機能
	QTranslator qTranslator;
	qTranslator.load("vs_disp_jp.qm", ":/res/translate");
//	if (!qTranslator.isEmpty())
//	{
//		qDebug("translator is not empty");
//		QString msg = qTranslator.translate("versionUp", "version_up_title");
//		qDebug(msg.toStdString().c_str());
//	}

	QTextCodec *t = QTextCodec::codecForName("UTF-8");//QTextCodec::codecForLocale() QTextCodec::codecForName("EUC-JP")
	QTextCodec::setCodecForCStrings(t);
	QTextCodec::setCodecForTr(t);
	QTextCodec::setCodecForLocale(t);

	QApplication app(argc, argv);
	app.installTranslator(&qTranslator);

	creatVerupToQtMessThread();
	createTestThreadForIpcSend();

    VersionUp verUp;
    verUp.show();

    app.exec();

    //close opened fifo fd
    close(readfifo);
    close(dummyfifo);
    // delete fifo file
    unlink(D_VERUP_MENU_FIFO_FILE);
}
//! [start application]
