# gmake-format makefile 

all:
	latex man.tex

bibtex:
	bibtex man

ps:
	dvips -Ppdf -o man.ps man.dvi

pdf:
	ps2pdf13 man.ps man.pdf


clean:	
	rm -f *.dvi
	rm -f *.aux
	rm -f *.log
	rm -f *.bbl
	rm -f *.blg
	rm -f *~
	rm -f *.toc

