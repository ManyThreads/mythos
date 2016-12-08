# files

VECTOR_FILES := \
$(patsubst %.svg,%.pdf,$(shell find . -type f -name '*.svg')) \
$(patsubst %.dia,%.pdf,$(shell find . -type f -name '*.dia'))

# patterns

%.pdf : %.svg
	inkscape --without-gui "$<" --export-pdf="$@"

%.svg : %.dia
	dia --filter=svg --export="$@" "$<"

# targets

vectors: $(VECTOR_FILES)
	echo $(sort $(VECTOR_FILES)) > vectors

clean: vectors
	rm `cat vectors`
	rm vectors
