/* Cong Trinh
 * Artificial Intelligence
 * 11/9/19
 */

// Agent.h

#ifndef AGENT_H
#define AGENT_H

#include "Action.h"
#include "Percept.h"
#include "WorldState.h"
#include <list>

#include "Location.h"

class Agent
{
public:
	Agent();
	~Agent();
	void Initialize();
	Action Process(Percept &percept);
	void GameOver(int score);

	void UpdateState(Percept &percept);

	WorldState worldState;
	list<Action> actionList;
	Action previousAction;

	void UpdatePossibleWumpusLocations();
	void SetGoForward(Location &location);
	void SetGoLeft(Location &location);
	void SetGoRight(Location &location);
	void AddToPath(Location &location);
	Action ChooseAction(Percept &percept);
	void AddActionsFromPath(bool forward);
	bool LocationInList(list<Location> &locationList, const Location &location);
	void AddNewLocation(list<Location> &locationList, const Location &location);
	void AddAdjacentLocations(list<Location> &locationList, const Location &location);
	bool OutsideWorld(Location &location);
	void FilterSafeLocations();
	void Output();
	void PrintProbPit();
	void AddFrontier(list<Location> &locationList, const Location &location);
	void ComputeAllPits(Percept &percept);
	void CheckFrontier(int x, int y);
	bool SearchListLoc(list<Location> &list_loc, Location &loc);
	list<Location> FindFrontier();
	bool RemoveLoc(list<Location> &list_loc, Location &loc);
	bool AdjBreeze(Location &loc);
	float FindTruthTable(int size, list<Location> &L, Location &pit_x_y, bool is_xy_pit);
	float ComputeFrontierSum();
	bool AdjList(list<Location> &L, Location &loc);
	float get_pit_prob(Location &loc);
	void Pit(int x, int y, float v);

	list<Location> pathToGold;
	list<Location> stenchLocations;
	list<Location> clearLocations;
	list<Location> possibleWumpusLocations;
	list<Location> safeLocations;
	list<Location> visitedLocations;
	list<Location> frontier;
	list<Location> breezeList;
	list<Location> pitList;
	list<Location> possiblePitLoc;
	list<Location> knownPitLoc;

	bool firstTry;
	float prob_pit[5][5];
};

#endif // AGENT_H
