ifndef ANDROID_SYSROOT
SYSROOT  = $(HOME)/android/ndk/platforms/android-18/arch-arm/
else
SYSROOT  = $(ANDROID_SYSROOT)
endif

ifndef ANDROID_STLPORT_INCLUDES
STLPORT_INC = $(HOME)/android/ndk/sources/cxx-stl/stlport/stlport/
else
STLPORT_INC = $(ANDROID_STLPORT_INCLUDES)
endif

ifndef ANDROID_STLPORT_LIBS
STLPORT_LIBS = $(HOME)/android/ndk/sources/cxx-stl/stlport/libs/armeabi/
else
STLPORT_LIBS = $(ANDROID_STLPORT_LIBS)
endif

PREFIX     = arm-linux-androideabi-

CXX        = $(PREFIX)g++

CXXFLAGS   = -I$(STLPORT_INC) -L$(STLPORT_LIBS) -w -O3 -fPIC -fPIE -fpermissive -pie --sysroot $(SYSROOT)

MAIN_SRCS = main.cpp
MAIN_OBJS = $(MAIN_SRCS:.cpp=.o)

all: $(MAIN_OBJS)
	$(CXX) $(CXXFLAGS) -o arm_inject $(MAIN_OBJS) -lstlport_static

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	-rm -f *.o arm_inject
