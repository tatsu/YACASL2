#ifndef YACASL2_CASL2_INCLUDED
#define YACASL2_CASL2_INCLUDED

#include <time.h>
#include "word.h"

/**
 * @brief COMET IIの規格値
 */
enum {
    CMDSIZE = 4,              /**<命令の最大文字数 */
    GRSIZE = 8,               /**<汎用レジスタの数。COMET II規格では、8（0から7） */
    DEFAULT_MEMSIZE = 512,    /**<デフォルトのメモリ容量。COMET II規格では、65535語（word） */
    DEFAULT_CLOCKS = 5000000, /**<デフォルトのクロック周波数。COMET II規格では、未定義 */
};

/**
 * @brief COMET II フラグのマスク値
 */
enum {
    OF = 0x4,    /**<Overflow Flag */
    SF = 0x2,    /**<Sign Flag */
    ZF = 0x1,    /**<Zero Flag */
};

/**
 * @brief COMET IIのCPUを表すデータ型
 */
typedef struct {
    WORD gr[GRSIZE]; /**<汎用レジスタ */
    WORD sp;         /**<スタックポインタ */
    WORD pr;         /**<プログラムレジスタ */
    WORD fr;         /**<フラグレジスタ */
} CPU;

/**
 * @brief COMET IIの仮想実行マシンシステムを表すデータ型
 */
typedef struct {
    CPU *cpu;        /**<CPU */
    WORD *memory;    /**<メモリ */
    int memsize;     /**<メモリサイズ */
    clock_t clocks;  /**<クロック周波数 */
} SYSTEM;

/**
 * @brief COMET IIの仮想実行マシンシステム
 */
extern SYSTEM *sys;

/**
 * COMET II 命令
 * 命令タイプは、オペランドにより5種類に分類
 */
typedef enum {
    /**
     *  オペランド数2または3
     *  - 第1オペランド: 汎用レジスタ。必須
     *  - 第2オペランド: アドレス。必須
     *  - 第3オペランド: 指標レジスタとして用いる汎用レジスタ。任意
     */
    R_ADR_X = 01,
    /**
     *  オペランド数2
     *  - 第1オペランド: 汎用レジスタ。必須
     *  - 第2オペランド: 汎用レジスタ。必須
     */
    R1_R2 = 02,
    /**
     * オペランド数1または2
     * - 第1オペランド: アドレス。必須
     * - 第2オペランド: 指標レジスタとして用いる汎用レジスタ。任意
     */
    ADR_X = 03,
    /**
     * オペランド数1
     * - 第1オペランド: 汎用レジスタ。必須
     */
    R_ = 04,
    /**
     * オペランドなし
     */
    NONE = 00,
} CMDTYPE;

/**
 * 命令コード表の項目
 */
typedef struct {
    char *name;                 /**<命令名 */
    const void (*ptr);          /**<命令の関数ポインタ */
} CMD;

/**
 * COMET II命令コード表の項目
 */
typedef struct {
    char *name;                 /**<命令名 */
    CMDTYPE type;               /**<命令タイプ */
    WORD code;                  /**<命令コード */
    const void (*ptr);          /**<命令の関数ポインタ */
} COMET2CMD;

/**
 * 命令コードのハッシュ表
 */
typedef struct _CMDTAB {
    struct _CMDTAB *next;       /**<リスト次項目へのポインタ */
    COMET2CMD *cmd;             /**<命令コード表の項目 */
} CMDTAB;

/**
 * プログラム実行時の開始と終了のアドレス
 */
typedef struct {
    WORD start;   /**<開始アドレス */
    WORD end;     /**<終了アドレス */
    bool stop;    /**<終了フラグ */
} EXECPTR;

extern EXECPTR *execptr;

/**
 * COMET II仮想マシンのリセット
 */
void reset(int memsize, int clocks);

/**
 * COMET II仮想マシンのシャットダウン
 */
void shutdown();

/**
 * 名前とタイプがキーの命令ハッシュ表を作成する
 */
bool create_cmdtype_code();

/**
 * 命令の名前とタイプから、命令コードを返す
 * 無効な場合は0xFFFFを返す
 */
WORD getcmdcode(const char *cmd, CMDTYPE type);

/**
 * 名前とタイプがキーの命令ハッシュ表を解放する
 */
void free_cmdtype_code();

/**
 * コードがキーの命令ハッシュ表を作成する
 */
bool create_code_type();

/**
 * 命令コードから命令の関数ポインタを返す
 */
const void (*getcmdptr(WORD code));

/**
 * コードがキーの命令ハッシュ表を解放する
 */
void free_code_type();

#endif            /* YACASL2_CASL2_INCLUDEDの終端 */
