CFLAGS = -std=c++17 -O0 -g
raytracer: main.cpp
	g++ $(CFLAGS) -o raytracer main.cpp

clean:
	rm -f raytracer
