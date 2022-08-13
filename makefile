.PHONY : build

default :
	@echo "======================================="
	@echo "Please use 'make build' command to build it.."
	@echo "======================================="

CFLAGS = -O2 -Wall -shared -fPIC -Wl,-rpath,. -Wl,-rpath,..
CC = cc

INCLUDES = -I. -I../../src -I../../inc
LIBS = -L../ -L../../ -L../../../
DLL = -lcore

build:
	@$(CC) -o lmsgpack.so msgpack.c buf.c decode.c encode.c $(INCLUDES) $(LIBS) $(CFLAGS) $(DLL)
	@mv *.so ../
