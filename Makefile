all: weather

weather: weather.cpp
	g++ -Wall -Werror -std=c++11 -g weather.cpp -o weather 

run: weather
	./weather

.PHONY:clean

clean:
	-rm -rf weather
