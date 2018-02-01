/**
 * versionUp.cpp for G3NVR
 * Copyright(C) 2002-2017 MegaChips Co.,Ltd All right reserved.
 * author: "Cao Tianyang"<caoty@hyron-js.com>
 */

#include <QtGui>
#include <QScreen>

#include <errno.h>
#include <string.h>
#include "vs_disp_menu.h"

/**デジタルモニターの4Kモードは、3840 * 2160 */
#define HD_WIDTH_4K         ( 3840 )
#define HD_HEIGHT_4K        ( 2160 )
/**デジタルモニターのFULLHDモードは、1920 * 1080 */
#define HD_WIDTH_FULLHD     ( 1920 )
#define HD_HEIGHT_FULLHD    ( 1080 )
/**デジタルモニターのXGAモードは、1024 * 768 */
#define HD_WIDTH_XGA        ( 1024 )
#define HD_HEIGHT_XGA       (  768 )

/**画面UIに関する設定 */
#define COLOR_DEPTH	8
#define VERUP_BACKGROUND_COLOR				33,33,33			// 背景色
#define VERUP_FONT_NAME_PRO					"RgPHeiseiMG-W4"	// フォントタイプ
#define VERUP_FONT_SIZE						30					// フォントサイズ
#define VERUP_TITLE_MARGIN_BOTTOM			10					// タイトルのボトムマージンサイズ
#define VERUP_NVR_INFO_MARGIN_LEFT_RIGHT	150					// NVRバージョンアップ情報の左/右のマージンサイズ
#define VERUP_OTHER_INFO_MARGIN_LEFT_RIGHT	20					// NVR以外バージョンアップ情報の左/右のマージンサイズ
#define VERUP_OTHER_INFO_MARGIN_MIDDLE		10					// NVR以外バージョンアップ情報の真ん中のマージンサイズ

#define VERUP_LABEL_STYLE_STR				"QLabel {background-color:33,33,33; color:white;}"	// 画面ラベルのスタイル設定
#define VERUP_ROW_MFGNO_DIGIT_SUM			10000000			// バージョンアップカメラのmfgNoの桁数計算用
#define VERUP_ROW_MFGNO_DIGIT				7					// バージョンアップカメラのmfgNoの桁数、7桁

/**ipc受信用スレッド */
pthread_t g_verUpToQtMessTid = -1;
/**fifoファイル用 */
int readfifo,dummyfifo;
/**qtシグナル送信用Widget */
QVerupThreadWidget* g_verupToQtMsgThread;

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: ipcメッセージ受信用スレッド作成
// 戻り値   : true	: 成功
//			: false	: 失敗
////////////////////////////////////////////////////////////////////////////////
bool creatVerupToQtMessThread()
{
	int rc = pthread_create(&g_verUpToQtMessTid, NULL, qtRecvFrVsMain, NULL);
	if( 0 != rc )
	{
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: ipcメッセージ受信、qtシグナル送信
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void* qtRecvFrVsMain(void*)
{
	int readfifo;
	char qtRecvMsgBuf[D_VERUP_MENU_MSG_SIZE];

	// FIFO作成
	if (mkfifo(D_VERUP_MENU_FIFO_FILE, D_VERUP_MENU_FILE_MODE) < 0 && (errno != EEXIST))
	{
		// 失敗
		return NULL;

	}
	// 読むためFIFOオープン
	readfifo = open(D_VERUP_MENU_FIFO_FILE, O_RDONLY, 0);
//	qDebug("open fd: %d errno: %d \n", readfifo, errno);
	// VSMAINプロセスがこのFIFOクローズすると、readfifoの読む処理を中断するので、これ回避のため以下の処理実行
	dummyfifo = open(D_VERUP_MENU_FIFO_FILE, O_WRONLY, 0);

	while (ipcFifoMsgRecv(readfifo, &qtRecvMsgBuf, sizeof(qtRecvMsgBuf)))
	{
		g_verupToQtMsgThread->dispatchMessToVerup(qtRecvMsgBuf);
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: fifoのメッセージ読み込む
// 戻り値   : true	: 成功
//			: false	: 失敗
////////////////////////////////////////////////////////////////////////////////
bool ipcFifoMsgRecv(
		int		readfifo,		// [I]FIFOのファイル名
		void	*p_msg,			// [O]受信メッセージバッファ
		int		p_msg_size		// [I]受信データサイズ
)
{
	int rc;

	memset(p_msg, 0, p_msg_size);
	//read the fifo
	rc = read(readfifo, p_msg, p_msg_size);
//	qDebug("read fifo size: %d \n", rc);
	if (rc != p_msg_size && rc != 0)
	{
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: バージョンアップ記録のステータス設定
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void setStatusStr(unsigned char status, QLabel *label)
{
//	qDebug("setStatusStr: %d", status);
	switch (status)
	{
	case D_VERUP_MENU_STATUS_NOT:
		label->setText(VersionUp::tr("Pending"));
		break;
	case D_VERUP_MENU_STATUS_DOING:
		label->setText(VersionUp::tr("Running"));
		break;
	case D_VERUP_MENU_STATUS_DONE:
		label->setText(VersionUp::tr("Finished"));
		break;
	case D_VERUP_MENU_STATUS_FAILURE:
		label->setText(VersionUp::tr("Failed"));
		break;
	default:
		break;
	}
}

// for test
void* ipcFifoMsgSend(void*)
{
	int writefifo,len;
	ST_VERUP_MENU_MSG_INFO_UPDATE msg1;
	ST_VERUP_MENU_MSG_FIRM_TITLE msg2;
	ST_VERUP_MENU_MSG_END msg3;
	ST_VERUP_MENU_MSG_INFO_UPDATE msg4;
	ST_VERUP_MENU_MSG_MENU_CLEAR msg5;
	ST_VERUP_MENU_MSG_INFO_UPDATE msg6;

	char machineCode[] = "CNV0170";
	unsigned char status[] = {3,2,1};
	unsigned char status1[] = {1,3,2};

	ST_MACHINE_INFO* machineInfo;
	ST_MACHINE_INFO* machineInfo2;
//	qDebug("start ipcFifoMsgSend");
	writefifo = open(D_VERUP_MENU_FIFO_FILE, O_WRONLY, 0);
	while(1)
	{
		sleep(2);

		memset(&msg2, 0, sizeof(msg2));
		msg2.cmdKind = D_VERUP_MENU_CMD_FIRM_TITLE;
		msg2.titleKind = D_VERUP_MENU_FIRM_TITLE_3_3;
		len = write(writefifo, &msg2, sizeof(msg2));
//		qDebug("write to fifo size: %d \n", len);

		//row1
		memset(&msg1, 0, sizeof(msg1));
		msg1.cmdKind = D_VERUP_MENU_CMD_INFO_UPDATE;
		msg1.showMode = D_VERUP_MENU_SHOW_MODE_2;
		msg1.line = '4';
		machineInfo = msg1.macInfo;
		machineInfo->mfgNo = 0;
		strncpy(machineInfo->machineCode, machineCode, sizeof(machineInfo->machineCode));
		strncpy((char*)machineInfo->status, (char*)status, sizeof(machineInfo->status));

		machineInfo2 = msg1.macInfo + 1;
		machineInfo2->mfgNo = 0;
		strncpy(machineInfo2->machineCode, machineCode, sizeof(machineInfo2->machineCode));
		strncpy((char*)machineInfo2->status, (char*)status1, sizeof(machineInfo2->status));
		len = write(writefifo, &msg1, sizeof(msg1));
//		qDebug("write to fifo size: %d \n", len);
		sleep(2);

		msg1.line = '4';
		strncpy((char*)machineInfo->status, (char*)status1, sizeof(machineInfo->status));
		write(writefifo, &msg1, sizeof(msg1));
		sleep(2);


		//row2
		memset(&msg4, 0, sizeof(msg4));
		msg4.cmdKind = D_VERUP_MENU_CMD_INFO_UPDATE;
		msg4.showMode = D_VERUP_MENU_SHOW_MODE_2;
		msg4.line = '5';
		machineInfo = msg4.macInfo;
		machineInfo->mfgNo = 46;
		strncpy(machineInfo->machineCode, machineCode, sizeof(machineInfo->machineCode));
		strncpy((char*)machineInfo->status, (char*)status, sizeof(machineInfo->status));


		machineInfo2 = msg4.macInfo + 1;
		machineInfo2->mfgNo = 67;
		strncpy(machineInfo2->machineCode, machineCode, sizeof(machineInfo2->machineCode));
		strncpy((char*)machineInfo2->status, (char*)status1, sizeof(machineInfo2->status));
		len = write(writefifo, &msg4, sizeof(msg4));
//		qDebug("write to fifo size: %d \n", len);
		sleep(2);

		msg4.line = '5';
		strncpy((char*)machineInfo->status, (char*)status1, sizeof(machineInfo->status));
		write(writefifo, &msg4, sizeof(msg4));
		sleep(2);

		//row1 upd
		machineInfo = msg1.macInfo;
		machineInfo->mfgNo = 10;
		machineInfo2 = msg1.macInfo + 1;
		machineInfo2->mfgNo = 30;
		write(writefifo, &msg1, sizeof(msg1));
		sleep(2);

		//clear
		memset(&msg5, 0, sizeof(msg5));
		msg5.cmdKind = D_VERUP_MENU_CMD_MENU_CLEAR;
		write(writefifo, &msg5, sizeof(msg5));
//		sleep(5);

		//title
		write(writefifo, &msg2, sizeof(msg2));
		//nvr row
		memset(&msg6, 0, sizeof(msg6));
		msg6.cmdKind = D_VERUP_MENU_CMD_INFO_UPDATE;
		msg6.showMode = D_VERUP_MENU_SHOW_MODE_1;
		msg6.line = '4';
		machineInfo = msg6.macInfo;
		machineInfo->mfgNo = 30;
		strncpy(machineInfo->machineCode, machineCode, sizeof(machineInfo->machineCode));
		strncpy((char*)machineInfo->status, (char*)status, sizeof(machineInfo->status));
		write(writefifo, &msg6, sizeof(msg6));
//		qDebug("write to fifo size: %d \n", len);
		sleep(2);

		//clear
		write(writefifo, &msg5, sizeof(msg5));
		//title
		write(writefifo, &msg2, sizeof(msg2));
		//other row
		write(writefifo, &msg1, sizeof(msg1));
		sleep(2);


		//process end
		memset(&msg3, 0, sizeof(msg3));
		msg3.cmdKind = D_VERUP_MENU_CMD_END;
		len = write(writefifo, &msg3, sizeof(msg3));
	}
	return NULL;
}

// for test
void createTestThreadForIpcSend()
{
	pthread_t tmpTid;
	pthread_create(&tmpTid, NULL, ipcFifoMsgSend, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: ipcメッセージ種別に応じるシグナル送信
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void QVerupThreadWidget::dispatchMessToVerup(char* qtRecvMsgBuf)
{
	unsigned char cmdKind;

	if( qtRecvMsgBuf == NULL )
		return;
	cmdKind = (unsigned char)*qtRecvMsgBuf;
	switch (cmdKind)
	{
	case D_VERUP_MENU_CMD_FIRM_TITLE:
	{
		emit verupFirmTitleSignal((ST_VERUP_MENU_MSG_FIRM_TITLE*)qtRecvMsgBuf);
		break;
	}
	case D_VERUP_MENU_CMD_INFO_UPDATE:
	{
		emit verupInfoUpdSignal((ST_VERUP_MENU_MSG_INFO_UPDATE*)qtRecvMsgBuf);
		break;
	}
	case D_VERUP_MENU_CMD_MENU_CLEAR:
	{
		emit verupMenuClearSignal();
		break;
	}
	case D_VERUP_MENU_CMD_END:
	{
		emit verupEndSignal();
		break;
	}
	default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: コンストラクター
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
VersionUp::VersionUp(QWidget *parent)
    : QWidget(parent)
    ,m_Top_Lbl(NULL)
    ,isDrawBegin(false)
    ,parentMiddleRowLayout(NULL)
    ,parentLeftRowLayout(NULL)
    ,parentRightRowLayout(NULL)
{
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(10000);

    g_verupToQtMsgThread = new QVerupThreadWidget();

    // initialize the signal and slots of verup
    connectSignalAndSlots();


    // QTでFHDに切替処理
    //QScreen *screen = QScreen::instance();
    //screen->setMode(HD_WIDTH_FULLHD, HD_HEIGHT_FULLHD, COLOR_DEPTH);

    // ウィンドウスタイル設定
    setWindowFlags(Qt::FramelessWindowHint);
    // FULLHDモード
    resize(800, 600);

	// ファームワェア更新(1/1)
	m_Top_Lbl = new LabelItem(this);

	QVBoxLayout *vLayout = new QVBoxLayout(this);
	vLayout->addWidget(m_Top_Lbl, 0, Qt::AlignTop|Qt::AlignHCenter);
	vLayout->addSpacing(VERUP_TITLE_MARGIN_BOTTOM);

	//paintVerupHeaderForNvr(vLayout);
	//paintVerupHeaderForOthers(vLayout);
}

VersionUp::~VersionUp()
{
	delete g_verupToQtMsgThread;
	g_verupToQtMsgThread = NULL;

    delete parentMiddleRowLayout;
    delete parentLeftRowLayout;
    delete parentRightRowLayout;
	parentMiddleRowLayout = NULL;
	parentLeftRowLayout = NULL;
	parentRightRowLayout = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: 背景色描画
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::paintEvent(QPaintEvent *)
{
    // 背景色設定
    QPainter painter(this);
    painter.setBrush(QBrush(QColor(VERUP_BACKGROUND_COLOR)));
    painter.drawRect(0, 0, this->width(), this->height());
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: バージョンアップ情報のヘッダー描画
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::paintVerupHeader(QLayout* parentLayout)
{
	QVBoxLayout *vLayout = dynamic_cast<QVBoxLayout*>(parentLayout);

	// Mfg.No.
	QLabel *header_1 = new LabelItem(this, "Mfg.No.");
	// 機器
	QLabel *header_2 = new LabelItem(this, tr("Machine"));
	// M
	QLabel *header_3 = new LabelItem(this, tr("M"));
	// K
	QLabel *header_4 = new LabelItem(this, tr("K"));
	// A
	QLabel *header_5 = new LabelItem(this, tr("A"));

	QHBoxLayout *hLayout = new QHBoxLayout();
	hLayout->addWidget(header_1, 0, Qt::AlignLeft);
	hLayout->addWidget(header_2, 0, Qt::AlignLeft);
	hLayout->addWidget(header_3, 0, Qt::AlignLeft);
	hLayout->addWidget(header_4, 0, Qt::AlignLeft);
	hLayout->addWidget(header_5, 0, Qt::AlignLeft);

	vLayout->addLayout(hLayout, 0);
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: バージョンアップ機器の情報描画
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::paintVerupRow(QLayout* parentLayout, ST_MACHINE_INFO* mess, QString idxStr)
{
    int size = 0;
    QLabel* searchLabel;
    QVBoxLayout *vLayout = dynamic_cast<QVBoxLayout*>(parentLayout);

    QString mfgNoStr = QString::number(mess->mfgNo + VERUP_ROW_MFGNO_DIGIT_SUM).right(VERUP_ROW_MFGNO_DIGIT);
    QString machineCodeStr = QString(mess->machineCode);
    unsigned char* statusArr = mess->status;
    unsigned char mch = *statusArr++;
    unsigned char kch = *statusArr++;
    unsigned char ach = *statusArr;

    searchLabel = this->findChild<QLabel *>("row_mfg_" + idxStr);
    // 新規
    if (searchLabel == NULL)
    {
        // MfgNo
        QLabel *row_1 = new LabelItem(this, mfgNoStr);
        row_1->setObjectName("row_mfg_" + idxStr);
        // 機器
        QLabel *row_2 = new LabelItem(this, machineCodeStr);
        row_2->setObjectName("row_machine_" + idxStr);
        // M
        QLabel *row_3 = new LabelItem(this);
        row_3->setObjectName("row_m_" + idxStr);
        setStatusStr(mch, row_3);
        // K
        QLabel *row_4 = new LabelItem(this);
        row_4->setObjectName("row_k_" + idxStr);
        setStatusStr(kch, row_4);
        // A
        QLabel *row_5 = new LabelItem(this);
        row_5->setObjectName("row_a_" + idxStr);
        setStatusStr(ach, row_5);

        QHBoxLayout *hLayout = new QHBoxLayout();
        hLayout->addWidget(row_1, 0, Qt::AlignLeft);
        hLayout->addWidget(row_2, 0, Qt::AlignLeft);
        hLayout->addWidget(row_3, 0, Qt::AlignLeft);
        hLayout->addWidget(row_4, 0, Qt::AlignLeft);
        hLayout->addWidget(row_5, 0, Qt::AlignLeft);

        size = vLayout->children().size();
        vLayout->insertLayout(size, hLayout, 0);

        // mfgNoは「0」の場合で非表示する
        if (mess->mfgNo == 0)
        {
            row_1->hide();
            row_2->hide();
            row_3->hide();
            row_4->hide();
            row_5->hide();
        }
    }
    // 更新
    else
    {
        QLabel *row1 = this->findChild<QLabel *>("row_mfg_" + idxStr);
        row1->setText(mfgNoStr);
        QLabel *row2 = this->findChild<QLabel *>("row_machine_" + idxStr);
        row2->setText(machineCodeStr);
        QLabel *row3 = this->findChild<QLabel *>("row_m_" + idxStr);
        setStatusStr(mch, row3);
        QLabel *row4 = this->findChild<QLabel *>("row_k_" + idxStr);
        setStatusStr(kch, row4);
        QLabel *row5 = this->findChild<QLabel *>("row_a_" + idxStr);
        setStatusStr(ach, row5);

        // mfgNoは「0」以外の場合で表示させる
        if (mess->mfgNo != 0 && row1->isHidden())
        {
            row1->show();
            row2->show();
            row3->show();
            row4->show();
            row5->show();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: １列のヘッダー場合でヘッダー描画
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::paintVerupHeaderForNvr(QLayout* parentLayout)
{
	QVBoxLayout *vLayout = dynamic_cast<QVBoxLayout*>(parentLayout);

	QVBoxLayout *vChildLayout = new QVBoxLayout();

	paintVerupHeader(vChildLayout);
	vChildLayout->addStretch(1);

	QHBoxLayout *hLayout = new QHBoxLayout();
	hLayout->addSpacing(VERUP_NVR_INFO_MARGIN_LEFT_RIGHT);
	hLayout->addLayout(vChildLayout);
	hLayout->addSpacing(VERUP_NVR_INFO_MARGIN_LEFT_RIGHT);

	vLayout->addLayout(hLayout);

	this->isDrawBegin = true;
	this->parentMiddleRowLayout = vChildLayout;
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: ２列のヘッダー場合でヘッダー描画
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::paintVerupHeaderForOthers(QLayout* parentLayout)
{
	QVBoxLayout *vLayout = dynamic_cast<QVBoxLayout*>(parentLayout);

	QVBoxLayout *vChildLayout1 = new QVBoxLayout();

	paintVerupHeader(vChildLayout1);
	vChildLayout1->addStretch(1);

	QVBoxLayout *vChildLayout2 = new QVBoxLayout();

	paintVerupHeader(vChildLayout2);
	vChildLayout2->addStretch(1);

	QHBoxLayout *hLayout = new QHBoxLayout();
	hLayout->addSpacing(VERUP_OTHER_INFO_MARGIN_LEFT_RIGHT);
	hLayout->addLayout(vChildLayout1);
	hLayout->addSpacing(VERUP_OTHER_INFO_MARGIN_MIDDLE);
	hLayout->addLayout(vChildLayout2);
	hLayout->addSpacing(VERUP_OTHER_INFO_MARGIN_LEFT_RIGHT);

	vLayout->addLayout(hLayout);

	this->isDrawBegin = true;
	this->parentLeftRowLayout = vChildLayout1;
	this->parentRightRowLayout = vChildLayout2;
}

void VersionUp::update()
{
	//QApplication::instance()->exit();
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: qtシグナルに応じる画面処理を設定する
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::connectSignalAndSlots()
{
	// シグナルクリア
	QObject::disconnect(g_verupToQtMsgThread, 0, 0, 0);

	// Firmタイトルシグナル
	QObject::connect(g_verupToQtMsgThread, SIGNAL(verupFirmTitleSignal(ST_VERUP_MENU_MSG_FIRM_TITLE*)),
		this, SLOT(verupFirmTitleSlot(ST_VERUP_MENU_MSG_FIRM_TITLE*)), Qt::BlockingQueuedConnection);
	// 情報更新シグナル
	QObject::connect(g_verupToQtMsgThread, SIGNAL(verupInfoUpdSignal(ST_VERUP_MENU_MSG_INFO_UPDATE*)),
		this, SLOT(verupInfoUpdSlot(ST_VERUP_MENU_MSG_INFO_UPDATE*)), Qt::BlockingQueuedConnection);
	// 画面クリアシグナル
	QObject::connect(g_verupToQtMsgThread, SIGNAL(verupMenuClearSignal()),
		this, SLOT(verupMenuClearSlot()), Qt::BlockingQueuedConnection);
	// 画面表示終了シグナル
	QObject::connect(g_verupToQtMsgThread, SIGNAL(verupEndSignal()),
		this, SLOT(verupEndSlot()), Qt::BlockingQueuedConnection);
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: Firmタイトルシグナル処理
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::verupFirmTitleSlot(ST_VERUP_MENU_MSG_FIRM_TITLE* mess)
{
    unsigned char titleKind;
    QString title = tr("version_up_title");
    QString pageInfo;
    if (mess == NULL)
    {
        return;
    }
    titleKind = mess->titleKind;
    switch (titleKind)
    {
    case D_VERUP_MENU_FIRM_TITLE_1_1:
    {
        pageInfo = QString(" (1/1)");
        break;
    }
    case D_VERUP_MENU_FIRM_TITLE_1_2:
    {
        pageInfo = QString(" (1/2)");
        break;
    }
    case D_VERUP_MENU_FIRM_TITLE_2_2:
    {
        pageInfo = QString(" (2/2)");
        break;
    }
    case D_VERUP_MENU_FIRM_TITLE_1_3:
    {
        pageInfo = QString(" (1/3)");
        break;
    }
    case D_VERUP_MENU_FIRM_TITLE_2_3:
    {
        pageInfo = QString(" (2/3)");
        break;
    }
    case D_VERUP_MENU_FIRM_TITLE_3_3:
    {
        pageInfo = QString(" (3/3)");
        break;
    }
    default:
        pageInfo = QString(" (1/1)");
        break;
    }
    m_Top_Lbl->setText(title + pageInfo);
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: 情報更新シグナル処理
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::verupInfoUpdSlot(ST_VERUP_MENU_MSG_INFO_UPDATE* mess)
{
    unsigned char showMode;
    QString line, leftLine, rightLine;
    if (mess == NULL)
    {
        return;
    }
    showMode = mess->showMode;
    line = mess->line;

    // バージョンアップ情報描画前にMKAヘッダー描画する
    if (!this->isDrawBegin)
    {
        if (showMode == D_VERUP_MENU_SHOW_MODE_1)
        {
            paintVerupHeaderForNvr(this->layout());
        }
        else if (showMode == D_VERUP_MENU_SHOW_MODE_2)
        {
            paintVerupHeaderForOthers(this->layout());
        }
    }

    // バージョンアップ情報描画する
    if (showMode == D_VERUP_MENU_SHOW_MODE_1)
    {
        paintVerupRow(this->parentMiddleRowLayout, mess->macInfo, line);
    }
    else if (showMode == D_VERUP_MENU_SHOW_MODE_2)
    {
        leftLine = line + "_0";
        rightLine = line + "_1";
        // 左の列
        paintVerupRow(this->parentLeftRowLayout, mess->macInfo, leftLine);
        // 右の列
        paintVerupRow(this->parentRightRowLayout, mess->macInfo + 1, rightLine);
    }
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: 画面クリアシグナル処理
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::verupMenuClearSlot()
{
    QVBoxLayout *vLayout = dynamic_cast<QVBoxLayout*>(this->layout());
    // 一番トップのレイアウトに全てのwidgetとlayoutをクリアする
    // widget、layout、spaceItem同時に削除の場合でSegmentFaultエラー生じるので、まずwidget削除し、次はlayout、spaceItem削除すると問題なさそう
    clearLayoutToInitiate(vLayout, true);
    clearLayoutToInitiate(vLayout, false);
    // clearLayoutToInitiateにm_Top_Lbl、parentMiddleRowLayout、parentLeftRowLayout、parentRightRowLayoutをクリアした
    // VersionUp初期状態に戻す
    this->isDrawBegin = false;
    this->parentMiddleRowLayout = NULL;
    this->parentLeftRowLayout = NULL;
    this->parentRightRowLayout = NULL;

    m_Top_Lbl = new LabelItem(this);
    m_Top_Lbl->setText("");

	vLayout->addWidget(m_Top_Lbl, 0, Qt::AlignTop|Qt::AlignHCenter);
	vLayout->addSpacing(VERUP_TITLE_MARGIN_BOTTOM);
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: レイアウトに全てのwidgetとlayoutをクリアする
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::clearLayoutToInitiate(QLayout* layout, bool delWidgets)
{
    QLayoutItem* item;
    QWidget* widget;
    QLayout* childLayout;
    int sum = layout->count();
    qDebug("layout sum : %d !!! \n", sum);
    while (1)
    {
        if (layout->count() == 0)
        {
            return;
        }
        item = layout->takeAt(0);

        widget = item->widget();
        childLayout = item->layout();

        if (widget && delWidgets)
        {
            widget->deleteLater();
            delete item;
        }
        else if (childLayout)
        {
            clearLayoutToInitiate(childLayout, delWidgets);
        }
        if (!delWidgets)
        {
            delete item;
        }
    }

//    while (1)
//    {
//        if (n==sum)
//        {
//            return;
//        }
//        item = layout->itemAt(n);
//        widget = item->widget();
//        childLayout = item->layout();
//        spaceItem = item->spacerItem();

//        if (widget)
//        {
//            //delete widget;
//            qDebug("find widget: %s !!! \n", widget->objectName().toStdString().c_str());
//        }
//        else if (childLayout)
//        {
//            qDebug("find layout!!! \n");
//            clearLayoutToInitiate(childLayout);
//        }
//        else if (spaceItem)
//        {
//            qDebug("find spaceItem!!! \n");
//        }
//        else
//        {
//            qDebug("find others!!! \n");
//        }
//        //delete item;
//        n++;
//    }
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: 画面表示終了シグナル処理
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
void VersionUp::verupEndSlot()
{
    QApplication::instance()->exit();
}

////////////////////////////////////////////////////////////////////////////////
// 外部関数
// 内容		: コンストラクタ
// 戻り値   :
////////////////////////////////////////////////////////////////////////////////
LabelItem::LabelItem(QWidget *parent, QString label)
    :QLabel(parent)
{
    int fontSize = VERUP_FONT_SIZE;
    QFont f(VERUP_FONT_NAME_PRO, fontSize);

	this->setAlignment(Qt::AlignCenter);
	this->setFont(f);
	this->setText(label);
	this->setStyleSheet(VERUP_LABEL_STYLE_STR);
}


