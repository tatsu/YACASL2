.PHONY: texinfo-distclean texinfo-clean texinfo-textmp-clean

CP := cp
DVIPDFMX := dvipdfmx
DVIPDFMX_FLAGS ?=
ECHO := echo
INSTALL-INFO := install-info
MAKEINFO_FLAGS ?=
MKDIR := mkdir
RMR := rm -rf
SED := sed
TEX := ptex
TEXI2DVI := TEX=$(TEX) texi2dvi
TEXI2DVI_FLAGS ?= -q --texinfo=@afourpaper
MV := mv

# \includeコマンドで読み込まれるファイル
intexi = $(strip $(shell $(SED) -n -e 's/@include \(.*\)/\1/pg' $<))

# \imageコマンドで読み込まれるファイル
imagetexi = $(strip $(shell $(SED) -n -e 's/.*@image{\([^,]*\)\(,[^,]*,[^,]*,[^,]*,\([^,}]*\)\)}.*/\1\3/p' $<))

# 依存関係を自動生成し、dファイルに格納
%.d: %.texi
	@$(ECHO) '$@ is created by scanning $<.'
    # texiファイルと出力ファイルの依存関係
	@$(ECHO) '$(foreach ext,.html _html .pdf .xml .info .txt .d,$(subst .texi,$(ext),$<)): $<' >$@
    # includeファイルの依存関係
	$(if $(intexi),@( \
      $(ECHO) >>$@; \
      $(ECHO) '# Include Files' >>$@; \
      $(ECHO) '$(foreach ext,.html _html .info .pdf .xml .txt,$<),$(subst .texi,$(ext),$<): $(intexi)') >>$@)
  # imageファイルの依存関係
  # Docbookは未定義
	$(if $(imagetexi),@( \
      $(ECHO) >>$@; \
      $(ECHO) '# Image Files: HTML, HTML split, Info' >>$@; \
      $(ECHO) '$(foreach ext,.html _html .info,$(subst .texi,$(ext),$<)): $(imagetexi)' >>$@))
	$(if $(imagetexi),@( \
      $(ECHO) >>$@; \
      $(ECHO) '# Imagefiles: DVI -> PDF' >>$@; \
      $(ECHO) '$(subst .texi,.pdf,$<): $(addsuffix .eps,$(basename $(imagetexi)))' >>$@))

# 変数TEXITARGETSで指定されたターゲットファイルに対応するdファイルをインクルード
# .dファイルからヘッダファイルの依存関係を取得する
# ターゲットに clean が含まれている場合は除く
ifeq (,$(filter %clean,$(MAKECMDGOALS)))
  -include $(addsuffix .d,$(basename $(TEXITARGETS)))
endif

%.info: %.texi
	$(MAKEINFO) $(MAKEINFO_FLAGS) -o $@ $<

%.html: %.texi
	$(MAKEINFO) -o $@ --no-split --html --css-include=$(CSS) $<

%_html: %.texi
	if test ! -e $@; then $(MKDIR) $@; fi
	$(CP) $(CSS) $@/
	$(MAKEINFO) -o $@ --html --css-ref=$(CSS) $<

%.html %_html: $(CSS)

%.pdf: %.dvi
	$(DVIPDFMX) $(DVIPDFMXFLAGS) $<

%.txt: %.texi
	$(MAKEINFO) --no-headers --disable-encoding -o $@ $<

%.xml: %.texi
	@$(MAKEINFO) --docbook -o $@ $<

texinfo-distclean: texinfo-clean
	$(RMR) *_html *.info *.html *.pdf  *.dvi *.txt

texinfo-clean: texinfo-textmp-clean
	$(RM) *.d

texinfo-textmp-clean:
	$(RM) *.aux *.cp *.cps *.fn *.ky *.log *.pg *.pgs *.tmp *.toc *.tp *.vr
