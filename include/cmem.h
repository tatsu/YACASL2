#ifndef YACASL2_CMEM_H_INCLUDED
#define YACASL2_CMEM_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

/* mallocを実行し、0で初期化 */
/* メモリを確保できない場合はエラーを出力して終了 */
void *malloc_chk(size_t size, char *tag);

/* callocを実行 */
/* メモリを確保できない場合はエラーを出力して終了 */
void *calloc_chk(size_t nmemb, size_t size, char *tag);

/* malloc_chkを実行してメモリを確保してから、 */
/* コピーした文字列を返す */
char *strdup_chk(const char *s, char *tag);

/* メモリがNULLの場合は解放 */
void free_chk(void *ptr, char *tag);
#endif