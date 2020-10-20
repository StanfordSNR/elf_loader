trial: trial.cpp.o elfloader.cpp.o wasm-rt-impl.o
	g++ -g -o trial trial.cpp.o elfloader.cpp.o wasm-rt-impl.o

trial.cpp.o: trial.cpp
	g++ -c -o trial.cpp.o trial.cpp

elfloader.cpp.o: elfloader.cpp
	g++ -c -o elfloader.cpp.o elfloader.cpp

wasm-rt-impl.o: wasm-rt/wasm-rt-impl.c
	gcc -c -o wasm-rt-impl.o wasm-rt/wasm-rt-impl.c

clean:
	rm -rf trial trial.cpp.o elfloader.cpp.o wasm-rt-impl.o
	
