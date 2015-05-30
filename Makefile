PROCESS=com.android.chrome
ACTIVITY=com.google.android.apps.chrome.Main

all:
	@ndk-build -B

clean:
	@rm -rf obj libs

test: all
	python test.py
