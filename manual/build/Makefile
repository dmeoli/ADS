# Makefile for the ADS manual (single source -> web + PDF).
# Run from manual/build/.
#
#   make pdf       # PDF via Pandoc + pdflatex         -> ../ADS-manual.pdf
#   make html      # static MkDocs site                -> ../../manual-site/
#   make serve     # live MkDocs preview at 127.0.0.1:8000
#   make figures   # (re)build each figure .pdf and .svg from its TikZ source
#   make snippets  # extract the code and the pseudocode of the headers
#   make check     # the bibliography and the map of chapter 1 are complete,
#                  # and the C++ blocks of the chapters match the headers
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
# the tests that run the examples of appendix C [see declcheck.py]
CTESTS  := $(MANUAL)/../test/interop_examples.cpp
CTESTS  := $(CTESTS),$(MANUAL)/../test/interop_examples_mpi.cpp
# lamalunga (github.com/dmeoli/lamalunga), which computes the palette of the
# figures; set LAMALUNGA to its folder if it is elsewhere
LAMALUNGA ?= $(HOME)/GDrive/lamalunga
FIGTEX  := $(filter-out $(FIGDIR)/tikz-ads.tex,$(wildcard $(FIGDIR)/*.tex))

.PHONY: pdf html serve figures snippets check clean

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

# every work the library cites is in the bibliography, every header and
# class of the library is in the map of the structures of chapter 1, and
# every declaration block of a chapter is in its header and every example
# compiles against the headers, those of appendix C also being in the tests
# that run them (declcheck.py)
check: snippets
	python3 bibcheck.py $(MANUAL)/..
	python3 structmap.py $(INCLUDE) --check $(MANUAL)/chapters/01-introduction.md
	python3 declcheck.py $(INCLUDE) $(MANUAL)/chapters \
	 --tests C-libraries.md:$(CTESTS)

clean:
	rm -rf ../../manual-site $(GENDIR)
	rm -f $(PDF_OUT)
