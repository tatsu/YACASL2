#include "casl2.h"
#include "exec.h"

/* 実行のエラー定義 */
CERR cerr_exec[] = {
    { 202, "SVC input - out of Input memory" },
    { 203, "SVC output - out of COMET II memory" },
    { 204, "Program Register (PR) - out of COMET II memory" },
    { 205, "Stack Pointer (SP) - cannot allocate stack buffer" },
    { 206, "Address - out of COMET II memory" },
    { 207, "Stack Pointer (SP) - out of COMET II memory" },
};

bool addcerrlist_exec()
{
    return addcerrlist(ARRAYSIZE(cerr_exec), cerr_exec);
}

/* 実行モード: trace, logical, dump */
EXECMODE execmode = {false, false, false};

/* 標準入力から文字データを読込（SVC 1） */
void svcin()
{
    int i;
    char *buffer = malloc_chk(INSIZE + 1,"svcin.buffer");

    if(fgets(buffer, INSIZE, stdin) == NULL) {
        sys->memory[sys->cpu->gr[1]] = sys->memory[sys->cpu->gr[2]] = 0x0;
        return;
    }
    for(i = 0; i < INSIZE; i++) {
        if(*(buffer + i) == '\0' || *(buffer + i) == '\n') {
            --i;
            break;
        }
        if(sys->cpu->gr[1] + i >= sys->memsize - 1) {
            setcerr(202, NULL);    /* SVC input - out of Input memory */
            break;
        }
        sys->memory[sys->cpu->gr[1]+i] = *(buffer + i);
    }
    sys->memory[sys->cpu->gr[2]] = i + 1;
}

/* 標準出力へ文字データを書出（SVC 2） */
void svcout()
{
    int i;
    WORD w;

    for(i = 0; i < sys->memory[sys->cpu->gr[2]]; i++) {
        if(sys->cpu->gr[1] + i >= sys->memsize - 1) {
            setcerr(203, NULL);    /* SVC output - out of Comet II memory */
            return;
        }
        /* 「文字の組」の符号表に記載された文字と、改行（CR）／タブを表示 */
        /* それ以外の文字は、「.」で表す */
        if(((w = sys->memory[sys->cpu->gr[1]+i]) >= 0x20 && w <= 0x7E) || w == 0xA || w == '\t') {
            putchar((char)w);
        } else {
            putchar('.');
        }
    }
}

/* ロード／論理積／論理和／排他的論理和のフラグ設定。OFは常に0 */
void setfr(WORD val)
{
    sys->cpu->fr = 0x0;
    /* 第15ビットが1のとき、SFは1 */
    if((val & 0x8000) > 0x0) {
        sys->cpu->fr += SF;
    }
    /* 演算結果が0のとき、ZFは1 */
    if(val == 0x0) {
        sys->cpu->fr += ZF;
    }
}

/* 算術加算。フラグを設定して値を返す */
WORD adda(WORD val0, WORD val1)
{
    WORD res;
    long temp;
    sys->cpu->fr = 0x0;

    temp = (signed short)val0 + (signed short)val1;
    if(temp > 32767 || temp < -32768) {
        sys->cpu->fr += OF;
    }
    if(((res = (WORD)(temp & 0xFFFF)) & 0x8000) == 0x8000) {
        sys->cpu->fr += SF;
    } else if(res == 0x0) {
        sys->cpu->fr += ZF;
    }
    return res;
}

/* 算術減算。フラグを設定して値を返す */
WORD suba(WORD val0, WORD val1)
{
    return adda(val0, (~val1 + 1));
}

/* 論理加算。フラグを設定して値を返す */
WORD addl(WORD val0, WORD val1)
{
    long temp;
    WORD res;
    sys->cpu->fr = 0x0;

    if((temp = val0 + val1) < 0 || temp > 65535) {
        sys->cpu->fr += OF;
    }
    if(((res = (WORD)(temp & 0xFFFF)) & 0x8000) == 0x8000) {
        sys->cpu->fr += SF;
    } else if(res == 0x0) {
        sys->cpu->fr += ZF;
    }
    return res;
}

/* 論理減算。フラグを設定して値を返す */
WORD subl(WORD val0, WORD val1)
{
    return addl(val0, (~val1 + 1));
}

/* 算術比較のフラグ設定。OFは常に0 */
void cpa(WORD val0, WORD val1)
{
    sys->cpu->fr = 0x0;
    if((short)val0 < (short)val1) {
        sys->cpu->fr = SF;
    } else if(val0 == val1) {
        sys->cpu->fr = ZF;
    }
}

/* 論理比較のフラグ設定。OFは常に0 */
void cpl(WORD val0, WORD val1)
{
    sys->cpu->fr = 0x0;
    if(val0 < val1) {
        sys->cpu->fr = SF;
    } else if(val0 == val1) {
        sys->cpu->fr = ZF;
    }
}

/* 算術左シフト。フラグを設定して値を返す。 */
/* 算術演算なので、第15ビットは送り出されない */
WORD sla(WORD val0, WORD val1)
{
    WORD sign, res, last = 0x0;
    int i;

    sys->cpu->fr = 0x0;
    sign = val0 & 0x8000;
    res = val0 & 0x7FFF;
    for(i = 0; i < val1; i++) {
        last = res & 0x4000;
        res <<= 1;
    }
    res = sign | (res & 0x7FFF);
    /* OFに、レジスタから最後に送り出されたビットの値を設定 */
    if(last > 0x0) {
        sys->cpu->fr += OF;
    }
    /* 符号（第15ビット）が1のとき、SFは1 */
    if(sign > 0x0) {
        sys->cpu->fr += SF;
    }
    /* 演算結果が0のとき、ZFは1 */
    if(res == 0x0) {
        sys->cpu->fr += ZF;
    }
    return res;
}

/* 算術右シフト。フラグを設定して値を返す */
/* 算術演算なので、第15ビットは送り出されない */
/* 空いたビット位置には符号と同じものが入る */
WORD sra(WORD val0, WORD val1)
{
    WORD sign, res, last = 0x0;
    int i;

    sys->cpu->fr = 0x0;
    sign = val0 & 0x8000;
    res = val0 & 0x7FFF;
    for(i = 0; i < val1; i++) {
        last = res & 0x1;
        res >>= 1;
        if(sign > 0) {
            res |= 0x4000;
        }
    }
    res = sign | res;
    /* OFに、レジスタから最後に送り出されたビットの値を設定 */
    if(last > 0x0) {
        sys->cpu->fr += OF;
    }
    /* 符号（第15ビット）が1のとき、SFは1 */
    if(sign > 0x0) {
        sys->cpu->fr += SF;
    }
    /* 演算結果が0のとき、ZFは1 */
    if(res == 0x0) {
        sys->cpu->fr += ZF;
    }
    return res;
}

/* 論理左シフト。フラグを設定して値を返す */
WORD sll(WORD val0, WORD val1)
{
    WORD res = val0, last = 0x0;
    int i;

    sys->cpu->fr = 0x0;
    for(i = 0; i < val1; i++) {
        last = res & 0x8000;
        res <<= 1;
    }
    /* OFに、レジスタから最後に送り出されたビットの値を設定 */
    if(last > 0x0) {
        sys->cpu->fr += OF;
    }
    /* 第15ビットが1のとき、SFは1 */
    if((res & 0x8000) > 0x0) {
        sys->cpu->fr += SF;
    }
    /* 演算結果が0のとき、ZFは1 */
    if(res == 0x0) {
        sys->cpu->fr += ZF;
    }
    return res;
}

/* 論理右シフト。フラグを設定して値を返す */
WORD srl(WORD val0, WORD val1)
{
    WORD res = val0, last = 0x0;
    int i;

    sys->cpu->fr = 0x0;
    for(i = 0; i < val1; i++) {
        last = res & 0x0001;
        res >>= 1;
    }
    /* OFに、レジスタから最後に送り出されたビットの値を設定 */
    if(last > 0x0) {
        sys->cpu->fr += OF;
    }
    /* 第15ビットが1のとき、SFは1 */
    if((res & 0x8000) > 0x0) {
        sys->cpu->fr += SF;
    }
    /* 演算結果が0のとき、ZFは1 */
    if(res == 0x0) {
        sys->cpu->fr += ZF;
    }
    return res;
}

/* 仮想マシンCOMET IIでの実行 */
bool exec()
{
    WORD op, r_r1, x_r2, val;
    CMDTYPE cmdtype;
    char *errpr = malloc_chk(CERRSTRSIZE + 1, "exec.errpr");
    clock_t clock_begin, clock_end;

    addcerrlist_exec();
    if(execmode.trace == true) {
        fprintf(stdout, "\nExecuting machine codes\n");
    }
    /* フラグレジスタの初期値設定 */
    sys->cpu->fr = 0x0;
    sys->cpu->sp = sys->memsize;
    sys->cpu->pr = prog->start;
    /* 機械語の実行 */
    for (; ; ) {
        clock_begin = clock();
        /* プログラムレジスタのアドレスが主記憶の範囲外の場合はエラー */
        if(sys->cpu->pr >= sys->memsize) {
            sprintf(errpr, "PR:#%04X", sys->cpu->pr);
            setcerr(204, errpr);    /* Program Register (PR) - out of COMET II memory */
        }
        /* スタック領域のアドレスが主記憶の範囲外の場合はエラー */
        if(sys->cpu->sp > sys->memsize) {
            sprintf(errpr, "PR:#%04X", sys->cpu->pr);
            setcerr(207, errpr);    /* Stack Pointer (SP) - out of COMET II memory */
        }
        /* スタック領域を確保できない場合はエラー */
        if(sys->cpu->sp <= prog->end) {
            sprintf(errpr, "PR:#%04X", sys->cpu->pr);
            setcerr(205, errpr);    /* Stack Pointer (SP) - cannot allocate stack buffer */
        }
        /* 命令の取り出し */
        op = sys->memory[sys->cpu->pr] & 0xFF00;
        /* 命令の解読 */
        cmdtype = getcmdtype(op);
        r_r1 = (sys->memory[sys->cpu->pr] >> 4) & 0xF;
        x_r2 = sys->memory[sys->cpu->pr] & 0xF;
        /* エラー発生時は終了 */
        if(cerr->num > 0) {
            goto execerr;
        }
        /* traceオプション指定時、レジスタを出力 */
        if(execmode.trace){
            fprintf(stdout, "#%04X: Register::::\n", sys->cpu->pr);
            dspregister();
        }
        /* dumpオプション指定時、メモリを出力 */
        if(execmode.dump){
            fprintf(stdout, "#%04X: Memory::::\n", sys->cpu->pr);
            dumpmemory();
        }
        /* どちらかのオプション指定時、改行を出力 */
        if(execmode.dump || execmode.trace) {
            fprintf(stdout, "\n");
        }
        sys->cpu->pr++;
        /* オペランドの取り出し */
        if(cmdtype == R1_R2) {
            assert(x_r2 < GRSIZE);
            val = sys->cpu->gr[x_r2];
        }
        else if(cmdtype ==  R_ADR_X || cmdtype == R_ADR_X_ || cmdtype == ADR_X) {
            assert(x_r2 < GRSIZE);
            /* 実効アドレス（値または値が示す番地）を取得  */
            val = sys->memory[sys->cpu->pr++];
            /* 指標アドレスを加算  */
            if(x_r2 > 0x0) {
                val += sys->cpu->gr[x_r2];
            }
            /* ロード／算術論理演算命令／比較演算命令では、アドレスに格納されている内容を取得 */
            if(cmdtype == R_ADR_X_) {
                if(val >= sys->memsize) {
                    sprintf(errpr, "PR:#%04X", sys->cpu->pr-1);
                    setcerr(206, errpr);    /* Address - out of COMET II memory */
                    goto execerr;
                }
                val = sys->memory[val];
            }
        }
        /* 主オペランドが1〜4の場合、第2ビットを無視 */
        if(op >= 0x1000 && op <= 0x4FFF) {
            op &= 0xFB00;
        }
        /* 命令の実行 */
        switch(op)
        {
        case 0x0:       /* NOP */
            break;
        case 0x1000:    /* LD */
            setfr(sys->cpu->gr[r_r1] = val);
            break;
        case 0x1100:    /* ST */
            sys->memory[val] = sys->cpu->gr[r_r1];
            break;
        case 0x1200:    /* LAD */
            sys->cpu->gr[r_r1] = val;
            break;
        case 0x2000:    /* ADDA */
            sys->cpu->gr[r_r1] = adda(sys->cpu->gr[r_r1], val);
            break;
        case 0x2100:    /* SUBA */
            sys->cpu->gr[r_r1] = suba(sys->cpu->gr[r_r1], val);
            break;
        case 0x2200:    /* ADDL */
            sys->cpu->gr[r_r1] = addl(sys->cpu->gr[r_r1], val);
            break;
        case 0x2300:    /* SUBL */
            sys->cpu->gr[r_r1] = subl(sys->cpu->gr[r_r1], val);
            break;
        case 0x3000:    /* AND */
            setfr(sys->cpu->gr[r_r1] &= val);
            break;
        case 0x3100:    /* OR */
            setfr(sys->cpu->gr[r_r1] |= val);
            break;
        case 0x3200:    /* XOR */
            setfr(sys->cpu->gr[r_r1] ^= val);
            break;
        case 0x4000:    /* CPA */
            cpa(sys->cpu->gr[r_r1], val);
            break;
        case 0x4100:    /* CPL */
            cpl(sys->cpu->gr[r_r1], val);
            break;
        case 0x5000:    /* SLA */
            sys->cpu->gr[r_r1] = sla(sys->cpu->gr[r_r1], val);
            break;
        case 0x5100:    /* SRA */
            sys->cpu->gr[r_r1] = sra(sys->cpu->gr[r_r1], val);
            break;
        case 0x5200:    /* SLL */
            sys->cpu->gr[r_r1] = sll(sys->cpu->gr[r_r1], val);
            break;
        case 0x5300:    /* SRL */
            sys->cpu->gr[r_r1] = srl(sys->cpu->gr[r_r1], val);
            break;
        case 0x6100:    /* JMI */
            if((sys->cpu->fr & SF) > 0) {
                sys->cpu->pr = val;
            }
            break;
        case 0x6200:    /* JNZ */
            if((sys->cpu->fr & ZF) == 0) {
                sys->cpu->pr = val;
            }
            break;
        case 0x6300:    /* JZE */
            if((sys->cpu->fr & ZF) > 0) {
                sys->cpu->pr = val;
            }
            break;
        case 0x6400:    /* JUMP */
            sys->cpu->pr = val;
            break;
        case 0x6500:    /* JPL */
            if((sys->cpu->fr & (SF | ZF)) == 0) {
                sys->cpu->pr = val;
            }
            break;
        case 0x6600:    /* JOV */
            if((sys->cpu->fr & OF) > 0) {
                sys->cpu->pr = val;
            }
            break;
        case 0x7000:    /* PUSH */
            assert(sys->cpu->sp > prog->end && sys->cpu->sp <= sys->memsize);
            sys->memory[--(sys->cpu->sp)] = val;
            break;
        case 0x7100:    /* POP */
            assert(sys->cpu->sp > prog->end && sys->cpu->sp <= sys->memsize);
            sys->cpu->gr[r_r1] = sys->memory[(sys->cpu->sp)++];
            break;
        case 0x8000:    /* CALL */
            assert(sys->cpu->sp > prog->end && sys->cpu->sp <= sys->memsize);
            sys->memory[--(sys->cpu->sp)] = sys->cpu->pr;
            sys->cpu->pr = val;
            break;
        case 0x8100:    /* RET */
            assert(sys->cpu->sp > prog->end && sys->cpu->sp <= sys->memsize);
            if(sys->cpu->sp == sys->memsize) {
                return true;
            } else {
                sys->cpu->pr = sys->memory[(sys->cpu->sp)++];
                break;
            }
        case 0xF000:    /* SVC */
            switch(val)
            {
            case 0x0: /* EXIT */
                return true;
            case 0x1: /* IN */
                svcin();
                break;
            case 0x2: /* OUT */
                svcout();
                break;
            }
        default:
            break;
        }
        do {
            clock_end = clock();
        } while(clock_end - clock_begin < CLOCKS_PER_SEC / sys->clocks);
    }
    return true;
execerr:
    fprintf(stderr, "Execute error - %d: %s\n", cerr->num, cerr->msg);
    return false;
}
