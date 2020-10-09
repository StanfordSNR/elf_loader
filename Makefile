trialmake: trial.cpp elfloader.cpp
	g++ -o trial trial.cpp elfloader.cpp -I. -g
