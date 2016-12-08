# files

VECTOR_FILES := \
$(patsubst %.svg,%.pdf,$(shell find . -type f -name '*.svg')) \
$(patsubst %.dia,%.pdf,$(shell find . -type f -name '*.dia'))

CUSTOM_TARGETS := manycore.pdf

# patterns

%.pdf : %.svg
	inkscape --without-gui "$<" --export-pdf="$@"

%.svg : %.dia
	dia --filter=svg --export="$@" "$<"

# targets

vectors: $(VECTOR_FILES) $(CUSTOM_TARGETS)
#	echo $(sort $(VECTOR_FILES)) > vectors

# custom targets

manycore.pdf: manycore.R manycore.csv
	Rscript manycore.R
