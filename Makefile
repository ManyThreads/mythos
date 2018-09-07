

# targets

all: 3rdparty/mcconf/mcconf kernel-amd64.log kernel-knc.log host-knc.log

3rdparty/mcconf/mcconf:
	git submodule init
	git submodule update
	3rdparty/mcconf/install-python-libs.sh

clean:
	rm -f *.log
	rm -rf kernel-amd64
	rm -rf kernel-knc
	rm -rf host-knc

# rules

%.log: %.config
	./3rdparty/mcconf/mcconf -i $<
