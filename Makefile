SUBDIRS=injector libhook

PROCESS=com.android.chrome
ACTIVITY=com.google.android.apps.chrome.Main

all:
	for d in $(SUBDIRS); do [ -d $$d ] && $(MAKE) -C $$d; done

clean:
	for d in $(SUBDIRS); do [ -d $$d ] && $(MAKE) -C $$d clean; done

test: all
	@echo "\n\n@ Preparing for testing ...\n"
	@adb push injector/injector /data/local/tmp/
	@adb push libhook/libhook.so /data/local/tmp/
	@adb shell chmod 777 /data/local/tmp/injector
	@echo "@ Attaching to process $(PROCESS) ..."
	@adb logcat -c
	@adb shell su -c am start $(PROCESS)/$(ACTIVITY)
	@sleep 2
	@adb shell su 0 setenforce 0
	@adb shell 'su -c "/data/local/tmp/injector `pidof $(PROCESS)` /data/local/tmp/libhook.so"'
	@adb logcat | grep HOOK
