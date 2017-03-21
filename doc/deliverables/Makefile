DOCS = \
	D2_2_Gesamtarchitektur.tex \
	D2_3_Architekturplan.tex \
	D3_2_Finaler_Prototyp.tex \
	D3_3_Entwicklungsplan.tex \
	D4_2_2_Developer_Tools.tex \
	D4_3_Developer_Cookbook.tex \
	D5_2_Infrastruktur_Evaluierung.tex

all: $(patsubst %.tex,%.pdf,$(DOCS))

%.pdf: %.tex header.tex literature.bib fig/*.pdf
	latexmk -pdf $*.tex

clean:
	latexmk -C $(DOCS)
