

# targets

all: 3rdparty/mcconf/mcconf.py kernel-amd64.log kernel-knc.log host-knc.log

3rdparty/mcconf/mcconf.py:
	git submodule init
	git submodule update
	3rdparty/mcconf/install-python-libs

clean:
	rm -f *.log
	rm -rf kernel-amd64
	rm -rf kernel-knc
	rm -rf host-knc

# rules

%.log: %.config
	./3rdparty/mcconf/mcconf.py -i $<
