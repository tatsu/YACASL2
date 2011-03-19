#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cerr.h"
#include "cmem.h"
#include "hash.h"
#include "assemble.h"

static int labelcnt = 0;                /* ラベル数 */
static LABELTAB *labels[LABELTABSIZE];  /* ラベル表 */

/**
 * ラベルのエラー定義
 */
static CERR cerr_label[] = {
    { 101, "label already defined" },
    { 102, "label table is full" },
    { 103, "label not found" },
};

/**
 * ラベルのエラーをエラーリストに追加
 */
void addcerrlist_label()
{
    addcerrlist(ARRAYSIZE(cerr_label), cerr_label);
}

/**
 * プログラム名とラベルに対応するハッシュ値を返す
 */
unsigned labelhash(const char *prog, const char *label)
{
    HKEY *keys[2];
    int i = 0, j;
    unsigned h;

    if(prog != NULL) {
        keys[i] = malloc_chk(sizeof(HKEY), "labelhash.key");
        keys[i]->type = CHARS;
        keys[i]->val.s = strdup_chk(prog, "labelhash.key.val");
        i++;
    }
    keys[i] = malloc_chk(sizeof(HKEY), "labelhash.key");
    keys[i]->type = CHARS;
    keys[i]->val.s = strdup_chk(label, "labelhash.key.val");
    h = hash(i+1, keys, LABELTABSIZE);
    for(j = 0; j < i + 1; j++) {
        FREE(keys[j]->val.s);
        FREE(keys[j]);
    }
    return h;
}

/**
 * プログラム名とラベルに対応するアドレスをラベル表から検索する
 */
WORD getlabel(const char *prog, const char *label)
{
    assert(label != NULL);
    LABELTAB *p;

    for(p = labels[labelhash(prog, label)]; p != NULL; p = p->next) {
        if((prog == NULL || (p->prog != NULL && strcmp(prog, p->prog) == 0)) &&
           strcmp(label, p->label) == 0)
        {
            return p->adr;
        }
    }
    return 0xFFFF;
}

/**
 * プログラム名、ラベル、アドレスをラベル表に追加する
 */
bool addlabel(const char *prog, const char *label, WORD adr)
{
    assert(label != NULL);
    LABELTAB *p;
    unsigned hashval;

    /* 登録されたラベルを検索。すでに登録されている場合はエラー発生 */
    if(getlabel(prog, label) != 0xFFFF) {
        setcerr(101, label);    /* label already defined */
        return false;
    }
    /* メモリを確保 */
    p = malloc_chk(sizeof(LABELTAB), "labels.next");
    /* プログラム名を設定 */
    if(prog == NULL) {
        p->prog = NULL;
    } else {
        p->prog = strdup_chk(prog, "labels.prog");
    }
    /* ラベルを設定 */
    p->label = strdup_chk(label, "labels.label");
    /* アドレスを設定 */
    p->adr = adr;
    /* ラベル数を設定 */
    labelcnt++;
    /* ハッシュ表へ追加 */
    hashval = labelhash(prog, label);
    p->next = labels[hashval];
    labels[hashval] = p;
    return true;
}

/**
 * ラベルを比較した結果を返す
 */
int compare_adr(const void *a, const void *b)
{
    return (**(LABELARRAY **)a).adr - (**(LABELARRAY **)b).adr;
}

/**
 * ラベル表を表示する
 */
void printlabel()
{
    int i, s = 0;
    LABELTAB *p;
    LABELARRAY **l;

    l = calloc_chk(labelcnt, sizeof(LABELARRAY **), "labels");
    for(i = 0; i < LABELTABSIZE; i++) {
        for(p = labels[i]; p != NULL; p = p->next) {
            assert(p->label != NULL);
            l[s] = malloc_chk(sizeof(LABELARRAY), "lables");
            if(p->prog == NULL) {
                l[s]->prog = NULL;
            } else {
                l[s]->prog = strdup_chk(p->prog, "labels.prog");
            }
            l[s]->label = strdup_chk(p->label, "labels.label");
            l[s++]->adr = p->adr;
        }
    }
    qsort(l, s, sizeof(*l), compare_adr);
    for(i = 0; i < s; i++) {
        if(l[i]->prog != NULL) {
            fprintf(stdout, "%s.", l[i]->prog);
        }
        fprintf(stdout, "%s ---> #%04X\n", l[i]->label, l[i]->adr);
    }
    FREE(l);
}

/**
 * ラベル表を解放する
 */
void freelabel()
{
    int i;
    LABELTAB *p, *q;

    for(i = 0; i < LABELTABSIZE; i++) {
        for(p = labels[i]; p != NULL; p = q) {
            q = p->next;
            FREE(p->prog);
            FREE(p->label);
            FREE(p);
        }
    }
}
