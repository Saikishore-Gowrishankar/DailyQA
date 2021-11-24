all: dailyqa docs
dailyqa:
	g++ DailyQA.cpp main.cpp -std=c++2a -Wall -Wextra -Weffc++ -pedantic -O2 -o DailyQA
	mv DailyQA ..
run:
	g++ DailyQA.cpp main.cpp -std=c++2a -Wall -Wextra -Weffc++ -pedantic -O2 -o DailyQA
	mv DailyQA ..
	cd ..; ./DailyQA 
docs:
	rm -r ../docs/*
	doxygen -u dconfig
	doxygen dconfig
	mv ../docs/html/* ../docs
exec:	
	cd ..; ./DailyQA
