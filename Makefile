SUBDIRS=injector libhook

PROCESS=com.android.chrome
ACTIVITY=com.google.android.apps.chrome.Main

all:
	for d in $(SUBDIRS); do [ -d $$d ] && $(MAKE) -C $$d; done

clean:
	for d in $(SUBDIRS); do [ -d $$d ] && $(MAKE) -C $$d clean; done

test: all
	python test.py
