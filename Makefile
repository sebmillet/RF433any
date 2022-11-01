ALL:

clean:
	make -C examples/01_main/ $@
	make -C examples/02_output-received-code/ $@
	make -C examples/03_output-signal-timings/ $@
	make -C examples/04_react_on_code/ $@
	make -C examples/05_callback/ $@
	make -C extras/testplan/test/ $@
	make -C extras/testplan/simul/ $@

mrproper:
	make -C examples/01_main/ $@
	make -C examples/02_output-received-code/ $@
	make -C examples/03_output-signal-timings/ $@
	make -C examples/04_react_on_code/ $@
	make -C examples/05_callback/ $@
	make -C extras/testplan/test/ $@
	make -C extras/testplan/simul/ $@

