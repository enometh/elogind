TARGETS = /usr/share/dbus-1/system-services/bestiae.sedem.Sessio1.service \
	/usr/share/dbus-1/system.d/bestiae.sedem.Sessio1.conf \
	/usr/local/bin/bestiae.sedem.Sessio1.py
SOURCES = $(notdir $(TARGETS))
PWD = $(shell pwd)

all: $(TARGETS)
clean:
	rm -fv $(TARGETS)

/usr/share/dbus-1/system-services/bestiae.sedem.Sessio1.service: bestiae.sedem.Sessio1.service
	cp -apfv $(PWD)/$< $@

/usr/share/dbus-1/system.d/bestiae.sedem.Sessio1.conf: bestiae.sedem.Sessio1.conf
	cp -apfv $(PWD)/$< $@

/usr/local/bin/bestiae.sedem.Sessio1.py: bestiae.sedem.Sessio1.py
	cp -apfv $(PWD)/$< $@

