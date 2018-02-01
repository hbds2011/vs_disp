/**
 * verup_menu_api.h for G3NVR
 * Copyright(C) 2002-2017 MegaChips Co.,Ltd All right reserved.
 * author: "Zhou Naijian"<zhounj@hyron-js.com>
 */

#ifndef _VERUP_MENU_API_H_
#define _VERUP_MENU_API_H_

/**メッセージサイズ */
#define D_VERUP_MENU_MSG_SIZE		(64)

/**firm種別個数 (M、K、A) */
#define	D_VERUP_MENU_FIRM_KIND_MAX	(3)

/**機器コード長 */
#define D_VERUP_MACCODE_LEN			(7)

/**FIFOファイル設定 */
#define	D_VERUP_MENU_FIFO_FILE  "/tmp/fifo_verup"
#define D_VERUP_MENU_FILE_MODE (S_IRUSR | S_IWUSR)

/** コマンド種別 */
enum
{
	D_VERUP_MENU_CMD_FIRM_TITLE = 0,		/** < Firmタイトル */
	D_VERUP_MENU_CMD_INFO_UPDATE,			/** < 情報更新 */
	D_VERUP_MENU_CMD_MENU_CLEAR,			/** < 画面クリア */
	D_VERUP_MENU_CMD_END,					/** < 画面表示終了 */
	D_VERUP_MENU_CMD_MAX
};

/** Firmタイトル : ファームウェア更新N/N */
enum
{
	D_VERUP_MENU_FIRM_TITLE_1_1 = 0,
	D_VERUP_MENU_FIRM_TITLE_1_2,
	D_VERUP_MENU_FIRM_TITLE_2_2,
	D_VERUP_MENU_FIRM_TITLE_1_3,
	D_VERUP_MENU_FIRM_TITLE_2_3,
	D_VERUP_MENU_FIRM_TITLE_3_3,
	D_VERUP_MENU_FIRM_TITLE_MAX
};

/** MKAタイトル */
enum
{
	D_VERUP_MENU_SHOW_MODE_1 = 0,			/** < 1列 */
	D_VERUP_MENU_SHOW_MODE_2,				/** < 2列 */
	D_VERUP_MENU_SHOW_MODE_MAX
};

/** 進捗状態 */
enum
{
	D_VERUP_MENU_STATUS_OUT = 0,			/** < -- */
	D_VERUP_MENU_STATUS_NOT,				/** < 未 */
	D_VERUP_MENU_STATUS_DOING,				/** < 中 */
	D_VERUP_MENU_STATUS_DONE,				/** < 完 */
	D_VERUP_MENU_STATUS_FAILURE,			/** < 失 */
	D_VERUP_MENU_STATUS_MAX
};

/** @struct ST_VERUP_MENU_MSG_FIRM_TITLE
 * メッセージ内容[ Firmタイトル ]
 */
typedef struct ST_VERUP_MENU_MSG_FIRM_TITLE_t
{
	unsigned char		cmdKind;			/** < コマンド種別 */
	unsigned char		titleKind;			/** < タイトル種別 */
	char				rsv[62];
} ST_VERUP_MENU_MSG_FIRM_TITLE;


/** @struct ST_VERUP_MENU_MSG_INFO_UPDATE
 * メッセージ内容[ 情報更新 ]
 */
typedef struct ST_MACHINE_INFO_t
{
	char				machineCode[D_VERUP_MACCODE_LEN + 1];		/** < 機器コード */
	unsigned long		mfgNo;									/** < MFGNO */
	unsigned char		status[D_VERUP_MENU_FIRM_KIND_MAX];		/** < 進捗状況 */
	char				rsv;
} ST_MACHINE_INFO;

typedef struct ST_VERUP_MENU_MSG_INFO_UPDATE_t
{
	unsigned char		cmdKind;			/** < コマンド種別 */
	unsigned char		showMode;			/** < 表示モード */
	unsigned char		line;				/** < 行号 (4 - 15) */
	char				rsv;
	ST_MACHINE_INFO		macInfo[2];			/** < 機器情報 */
	char				rsv2[28];
} ST_VERUP_MENU_MSG_INFO_UPDATE;

/** @struct ST_VERUP_MENU_MSG_MENU_CLEAR
 * メッセージ内容[ 画面クリア ]
 */
typedef struct ST_VERUP_MENU_MSG_MENU_CLEAR_t
{
	unsigned char		cmdKind;			/** < コマンド種別 */
	char				rsv[63];
} ST_VERUP_MENU_MSG_MENU_CLEAR;

/** @struct ST_VERUP_MENU_MSG_END
 * メッセージ内容[ 画面表示終了 ]
 */
typedef struct ST_VERUP_MENU_MSG_END_t
{
	unsigned char		cmdKind;			/** < コマンド種別 */
	char				rsv[63];
} ST_VERUP_MENU_MSG_END;


/** 画面メニュー表示API */
void verup_menu_init(void);
void verup_menu_exit(void);
void verup_menu_clear(void);
void verup_menu_print_info( ST_MACHINE_INFO *left, ST_MACHINE_INFO *right, unsigned char showMode, unsigned char line );
void verup_menu_print_title( unsigned char kind );

#endif	//_VERUP_MENU_API_H_
