CFLAGS = -O3 -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp

all:
	g++ $(CFLAGS) motioncomp.cpp -o motioncomp -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_video
	