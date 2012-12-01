
test1		:test1.cpp
	gcc $< -I/opt/ati-stream-sdk/include \
	       -I/opt/ati-stream-sdk/include/CAL \
	       -lOpenCL -lstdc++ -o $@
#	       -L/opt/ati-stream-sdk/lib 
