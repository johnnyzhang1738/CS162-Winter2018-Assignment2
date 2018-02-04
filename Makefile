all:
	${CXX} -std=c++11 -g main.cpp heap.cpp
clean:
	rm -f a.out
