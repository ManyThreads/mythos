
# rules

%.log: %.config
	./3rdparty/mcconf/mcconf.py --depsolve -i $<

# targets

all: kernel-amd64.log kernel-knc.log host-knc.log

clean:
	rm -f *.log
	rm -rf kernel-amd64
	rm -rf kernel-knc
	rm -rf host-knc
