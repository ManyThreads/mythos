all:
	latexmk -pdf main.tex

watch:
	latexmk -pdf -pvc main.tex

clean:
	latexmk -C make.tex

release: main.pdf
	cp -i main.pdf base-kernel.pdf
