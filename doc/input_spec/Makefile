#
#  Makefile: generating HTML and PDF from RST
#
#  NB: not a good solution, just an initial hack.

CBpath =./code-block
PY_CSS = --stylesheet=$(CBpath)/pygments-default.css
PY_STY = --stylesheet=$(CBpath)/pygments-default.sty

SRC = \
	AmanziNativeSpecV7.rst \
	AmanziInputSpec-v2.3.rst

HTML = $(SRC:%.rst=%.html)

PDF = $(SRC:%.rst=%.pdf)

# generic rules for targets

%.html : %.rst
	$(CBpath)/rst2html-pygments $(PY_CSS) $< $@ 

%.tex : %.rst
	$(CBpath)/rst2latex-pygments $(PY_STY) $< $@

%.pdf : %.tex
	pdflatex $<
	pdflatex $<
	pdflatex $<

html : $(HTML)

all : html pdf

pdf : $(PDF)

clean-html : 
	-/bin/rm -f *.html

clean-pdf :
	-/bin/rm -f *.tex *.pdf
	-/bin/rm -f *.aux *.toc *.log *.out

clean : clean-html clean-pdf
	-/bin/rm -f *~

distclean : clean
