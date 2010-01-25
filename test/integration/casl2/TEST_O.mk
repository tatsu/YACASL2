# テストのテンプレート
# make         : CMDで設定されたコマンドを実行した出力結果を1.txtに出力し、0.txtと比較し、レポート
# make check   : ↓
# make prepare : CMDで設定されたコマンドを実行した出力結果を0.txt（テストの想定結果）に出力
# make clean   : 「make」で生成されたファイルをクリア
# make cleanall: 「make」と「make clean」で生成されたファイルをクリア
LOGFILE = ../TEST.log
ERRFILE = err.txt
UNITNAME = `pwd | xargs basename`
CMD = $(CASL2) $(CASL2FLAG) $(ASFILE)
OD = od -t x2 $(OFILE)

.PHPNY: check prepare clean cleanall
check: clean report.txt
prepare: cleanall 0.txt
clean:
	@rm -f 1.txt diff.txt report.txt $(OFILE)
cleanall: clean
	@rm -f 0.txt
0.txt 1.txt: $(CASL2) $(ASFILE)
	@echo $(CMD) >$@; \
     $(CMD) >>$@ 2>$(ERRFILE); \
     echo  >>$@; \
     echo $(OD) >>$@; \
     $(OD)  >>$@ 2>>$(ERRFILE); \
     if test -s $(ERRFILE); then cat err.txt >>$@; else rm -f $(ERRFILE); fi
diff.txt: 1.txt
	@-diff 0.txt 1.txt >$@ 2>&1
report.txt: diff.txt
	@echo -n "$(UNITNAME): Test " >$@; \
     if test ! -s $^; then echo -n "Success " >>$@; rm -f $^; else echo -n "Failure " >>$@; fi; \
     echo `date +"%F %T"` >>$@