include ../common.mk

TARGET    = libhook.so
MAIN_SRCS = main.cpp hook.cpp
MAIN_OBJS = $(MAIN_SRCS:.cpp=.o)

all: $(MAIN_OBJS)
	@$(CXX) $(CXXFLAGS) -shared -Wl,-soname,$(TARGET) -o $(TARGET) $(MAIN_OBJS) -llog -lstlport_static

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	@rm -f *.o $(TARGET)
