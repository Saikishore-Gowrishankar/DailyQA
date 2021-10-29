all: dailyqa docs
dailyqa:
	g++ DailyQA.cpp -std=c++2a -Wall -Wextra -Weffc++ -pedantic -O2 -o DailyQA
	mv DailyQA ..
run:
	g++ DailyQA.cpp -std=c++2a -Wall -Wextra -Weffc++ -pedantic -O2 -o DailyQA
	mv DailyQA ..
	cd ..; ./DailyQA 
docs:
	rm -r ../docs/*
	doxygen dconfig
	mv ../docs/html/* ../docs
