

# targets

all: 3rdparty/mcconf/mcconf kernel-amd64.log kernel-knc.log host-knc.log kernel-ihk.log

3rdparty/mcconf/mcconf:
	git submodule update --init --recursive
	3rdparty/mcconf/install-python-libs.sh

clean:
	rm -f *.log
	rm -rf kernel-amd64
	rm -rf kernel-knc
	rm -rf host-knc
	rm -rf kernel-ihk

# rules

%.log: %.config
	./3rdparty/mcconf/mcconf -i $<
