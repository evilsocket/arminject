include ../common.mk

TARGET    = injector
MAIN_SRCS = main.cpp
MAIN_OBJS = $(MAIN_SRCS:.cpp=.o)

all: $(MAIN_OBJS)
	@$(CXX) $(CXXFLAGS) -o $(TARGET) $(MAIN_OBJS) -lstlport_static

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	@rm -f *.o $(TARGET)
