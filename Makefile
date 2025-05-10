CFLAGS = -std=c++17 -O3
raytracer: main.cpp
	g++ $(CFLAGS) -o raytracer main.cpp

clean:
	rm -f raytracer
