/**
 * versionUp.h for G3NVR
 * Copyright(C) 2002-2017 MegaChips Co.,Ltd All right reserved.
 * author: "Cao Tianyang"<caoty@hyron-js.com>
 */

#ifndef VERSIONUP_H
#define VERSIONUP_H

#include <pthread.h>

//for fifo
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

#include "verup_menu_api.h"

class QVerupThreadWidget;

/**ipc受信用スレッド */
extern pthread_t g_verUpToQtMessTid;
/**fifoファイル用 */
extern int readfifo,dummyfifo;
/**qtシグナル送信用Widget */
extern QVerupThreadWidget* g_verupToQtMsgThread;

// create thread for receiving ipc message
bool creatVerupToQtMessThread();
// execute receiving ipc message
void* qtRecvFrVsMain(void * arg);
// receive ipc message
bool ipcFifoMsgRecv(
		int		readfifo,		// [I]FIFOのファイル名
		void	*p_msg,			// [O]受信メッセージバッファ
		int		p_msg_size		// [I]受信メッセージバッファサイズ
);
void setStatusStr(unsigned char status, QLabel *label);

// for ipc message test
void* ipcFifoMsgSend(void* arg);
void createTestThreadForIpcSend();

class QVerupThreadWidget : public QWidget
{
	Q_OBJECT

public:
	QVerupThreadWidget(){}
	~QVerupThreadWidget(){}

	void dispatchMessToVerup(char* qtRecvMsgBuf);

public Q_SLOTS:

Q_SIGNALS:
	void verupFirmTitleSignal(ST_VERUP_MENU_MSG_FIRM_TITLE*);
	void verupInfoUpdSignal(ST_VERUP_MENU_MSG_INFO_UPDATE*);
	void verupMenuClearSignal();
	void verupEndSignal();
};

class VersionUp : public QWidget
{
    Q_OBJECT

public:
    explicit VersionUp(QWidget *parent = 0);
    ~VersionUp();

protected:
    void paintEvent(QPaintEvent *event);
    QLabel* m_Top_Lbl;

private:
    void connectSignalAndSlots();
    void paintVerupHeader(QLayout* parentLayout);
    void paintVerupRow(QLayout* parentLayout, ST_MACHINE_INFO* mess, QString idxStr);
    void paintVerupHeaderForNvr(QLayout* parentLayout);
    void paintVerupHeaderForOthers(QLayout* parentLayout);
    void clearLayoutToInitiate(QLayout* layout, bool delWidgets);

public Q_SLOTS:
    void update();
    void verupFirmTitleSlot(ST_VERUP_MENU_MSG_FIRM_TITLE*);
    void verupInfoUpdSlot(ST_VERUP_MENU_MSG_INFO_UPDATE*);
    void verupMenuClearSlot();
    void verupEndSlot();

private:
    bool isDrawBegin;
    QVBoxLayout* parentMiddleRowLayout;
    QVBoxLayout* parentLeftRowLayout;
    QVBoxLayout* parentRightRowLayout;
};

class LabelItem : public QLabel
{
    Q_OBJECT

public:
    LabelItem(QWidget *parent = 0, QString label = "");
};

#endif
