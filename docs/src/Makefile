memoria.ps: *.tex memoria.bib
	latex memoria.tex
	bibtex -min-crossrefs 0 memoria
	latex memoria.tex
	latex memoria.tex
	dvips -o memoria.ps memoria.dvi

ispell: *.tex
	ispell -T latin1 *.tex

clean:
	rm -f *.aux *.toc *.log *~ memoria.ps memoria.dvi *.bbl *.blg
