all:
	@ndk-build -B

clean:
	@rm -rf obj libs

test: all
	python test.py
