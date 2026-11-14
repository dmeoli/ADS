# Makefile for the ADS manual (single source -> web + PDF).
# Run from manual/build/.
#
#   make pdf       # PDF via Pandoc + pdflatex         -> ../ADS-manual.pdf
#   make html      # static MkDocs site                -> ../../manual-site/
#   make serve     # live MkDocs preview at 127.0.0.1:8000
#   make figures   # (re)build each figure .pdf and .svg from its TikZ source
#   make snippets  # extract the code and the pseudocode of the headers
#   make clean     # remove build artefacts
#
# Prerequisites:
#   pandoc, a LaTeX distribution (pdflatex + the usual packages),
#   python3 with mkdocs + mkdocs-material + pymdown-extensions,
#   poppler-utils (pdftocairo) for figure conversion.

MANUAL  := ..
FIGDIR  := $(MANUAL)/figures
GENDIR  := $(MANUAL)/generated
PDF_OUT := $(MANUAL)/ADS-manual.pdf
INCLUDE := $(MANUAL)/../include
# lamalunga (github.com/dmeoli/lamalunga), which computes the palette of the
# figures; set LAMALUNGA to its folder if it is elsewhere
LAMALUNGA ?= $(HOME)/GDrive/lamalunga
FIGTEX  := $(filter-out $(FIGDIR)/tikz-ads.tex,$(wildcard $(FIGDIR)/*.tex))

.PHONY: pdf html serve figures snippets clean

pdf: figures snippets
	pandoc --defaults=pandoc.yaml -o $(PDF_OUT)
	@echo "PDF written to $(PDF_OUT)"

html: figures snippets
	mkdocs build -f mkdocs.yml
	@echo "Static site written to ../../manual-site/"

serve: figures snippets
	mkdocs serve -f mkdocs.yml

# Each authored figure is a standalone TikZ file, built to .pdf (for the
# PDF) and converted to .svg (for the web) when its source is newer.
figures: $(FIGTEX:.tex=.pdf) $(FIGTEX:.tex=.svg)

.PRECIOUS: $(FIGDIR)/%.pdf

# SOURCE_DATE_EPOCH and FORCE_SOURCE_DATE make the PDF of a figure the same
# bytes at each build, so that rebuilding unchanged figures changes no file
$(FIGDIR)/%.pdf: $(FIGDIR)/%.tex $(FIGDIR)/tikz-ads.tex
	cd $(FIGDIR) && SOURCE_DATE_EPOCH=0 FORCE_SOURCE_DATE=1 \
	  TEXINPUTS=$(LAMALUNGA)//: pdflatex -interaction=nonstopmode -halt-on-error \
	  $*.tex > /dev/null && rm -f $*.aux $*.log

$(FIGDIR)/%.svg: $(FIGDIR)/%.pdf
	pdftocairo -svg $< $@

# The code between the markers //! [name] and the pseudocode of the
# comments of the headers, which the chapters include with --8<--.
snippets:
	python3 snarf.py $(INCLUDE) $(GENDIR)

clean:
	rm -rf ../../manual-site $(GENDIR)
	rm -f $(PDF_OUT)
