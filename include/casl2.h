#ifndef YACASL2_CASL2_INCLUDED
#define YACASL2_CASL2_INCLUDED

#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "word.h"
#include "hash.h"
#include "cerr.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

/* COMET IIの規格 */
enum {
    CMDSIZE = 4,      /* 命令の最大文字数 */
    REGSIZE = 8,      /* 汎用レジスタの数 */
    DEFAULT_MEMSIZE = 512,    /* デフォルトのメモリ容量。COMET II規格では、65536語 */
    DEFAULT_CLOCKS = 5000000, /* デフォルトのクロック周波数。COMET II規格では、未定義 */
};

/* COMET IIのメモリ */
extern WORD *memory;

/* COMET IIのCPUレジスタ */
extern WORD GR[REGSIZE], SP, PR, FR;

/* COMET II フラグのマスク値 */
enum {
    OF = 0x4,    /* Overflow Flag */
    SF = 0x2,    /* Sign Flag */
    ZF = 0x1,    /* Zero Flag */
};

/* レジストリの内容を表示する場合はTRUE */
extern bool tracemode;

/* レジストリの内容を論理値（0〜65535）で表示する場合はTRUE */
extern bool logicalmode;

/* メモリの内容を表示する場合はTRUE */
extern bool dumpmode;

/* ソースを表示する場合はTRUE */
extern bool srcmode;

/* ラベル表を表示する場合はTRUE */
extern bool labelmode;

/* ラベル表を表示して終了する場合はTRUE */
extern bool onlylabelmode;

/* アセンブラ詳細結果を表示するならTRUE */
extern bool asdetailmode;

/* アセンブルだけを行う場合はTRUE */
extern bool onlyassemblemode;

/* メモリーサイズ */
extern int memsize;

/* クロック周波数 */
extern int clocks;

/* 実行開始番地 */
extern WORD startptr;

/* 実行終了番地 */
extern WORD endptr;

/* COMET II 命令
   命令タイプは、オペランドにより6つに分類
       R_ADR_X = 010: オペランド数2または3。
                第1オペランドは汎用レジスタ、第2オペランドはアドレス、第3オペランドは指標レジスタ
       R_ADR_X_ = 011: 同上。ただし、実効アドレスに格納されている内容を示す
       R1_R2 = 020: オペランド数2。第1オペランド、第2オペランドともに汎用レジスタ
       ADR_X = 030: オペランド数1または2。第1オペランドはアドレス、第2オペランドは指標レジスタ
       R_ = 040: オペランド数1。第1オペランドはGR
       NONE = 0: オペランドなし
*/
typedef enum {
    R_ADR_X = 010,
    R_ADR_X_ = 011,
    R1_R2 = 020,
    ADR_X = 030,
    R_ = 040,
    NONE = 0,
} CMDTYPE;

typedef struct {
    char *cmd;
    CMDTYPE type;
    WORD code;
} CMDCODEARRAY;

/* 命令コードのハッシュ表 */
typedef struct _CMDCODETAB {
    struct _CMDCODETAB *next;
    CMDCODEARRAY *cca;
} CMDCODETAB;

/* 命令と命令タイプがキーのハッシュ表を作成する */
bool create_cmdtype_code();

/* 命令と命令タイプから、命令コードを取得する。
   無効な場合は0xFFFFを返す */
WORD getcmdcode(const char *cmd, CMDTYPE type);

/* 命令と命令タイプからハッシュ値を生成する */
unsigned hash_cmdtype(const char *cmd, CMDTYPE type);

/* 命令と命令タイプがキーのハッシュ表を表示する */
void print_cmdtype_code();

/* 命令と命令タイプがキーのハッシュ表を解放する */
void free_cmdtype_code();

/* 命令コードがキーのハッシュ表を作成する */
bool create_code_type();

/* 命令コードからハッシュ値を生成する */
unsigned hash_code(WORD code);

/* 命令コードから命令タイプを取得する。
   無効な場合はNONEを返す */
CMDTYPE getcmdtype(WORD code);

/* 命令コードがキーのハッシュ表を表示する */
void print_code_type();

/* 命令コードがキーのハッシュ表を解放する */
void free_code_type();

/* 指定されたファイルにアセンブル結果を書込 */
void outassemble(char *file);

#endif
