/* Cong Trinh
 * Artificial Intelligence
 * 11/9/19
 */

// Agent.cc

#include <iostream>
#include <iomanip>
#include <vector>
#include "Agent.h"

#include "Action.h"
#include "Location.h"
#include "Orientation.h"


#include <algorithm>

using namespace std;

Agent::Agent()
{
	firstTry = true;
}

Agent::~Agent()
{
}

void Agent::Initialize()
{
	cout << setprecision(2) << fixed; // Sets decimal output by 2 places
	worldState.agentOrientation = RIGHT;
	worldState.agentHasArrow = true;
	worldState.agentHasGold = false;
	actionList.clear();
	previousAction = CLIMB;   // dummy action to start
	worldState.worldSize = 5; // Is given all maps will be 5x5

	if (firstTry)
	{
		worldState.goldLocation = Location(0, 0); // unknown
		//worldState.worldSize = 0;				  // unknown
		pathToGold.push_back(Location(1, 1)); // path starts in (1,1)

		/// Initialize 5x5 Prob Locations (Default: 0.20)
		for (int x = 0; x < 5; x++)
			for (int y = 0; y < 5; y++)
				prob_pit[x][y] = 0.20;
	}
	else
	{
		if (worldState.goldLocation == Location(0, 0))
		{
			// Didn't find gold on first try (should never happen, but just in case)
			pathToGold.clear();
			pathToGold.push_back(Location(1, 1));
		}
		else
		{
			AddActionsFromPath(true); // forward through path from (1,1) to gold location
		}
		if (worldState.agentLocation.X != 1 && worldState.agentLocation.Y != 1)
		{
			SetGoForward(worldState.agentLocation);
			Pit(worldState.agentLocation.X, worldState.agentLocation.Y, 1.00);
			cout << "DIED - MARKING (" << worldState.agentLocation.X - 1 << ", " << worldState.agentLocation.Y - 1 << ") AS A PIT." << endl;
			cout << "(" << worldState.agentLocation.X - 1 << ", " << worldState.agentLocation.Y - 1 << ") = " << prob_pit[worldState.agentLocation.X - 1][worldState.agentLocation.Y - 1] << endl;
		}
	}
	worldState.agentLocation = Location(1, 1);
}

Action Agent::Process(Percept &percept)
{
	UpdateState(percept);
	//PrintProbPit();	// debug

	ComputeAllPits(percept);

	if (actionList.empty())
	{ // Reverse back to Goal
		if (percept.Glitter)
		{
			actionList.push_back(GRAB);
			AddActionsFromPath(false); // reverse path from (1,1) to gold location
		}
		else if (worldState.agentHasGold && (worldState.agentLocation == Location(1, 1)))
		{
			actionList.push_back(CLIMB);
		}
		else
		{
			actionList.push_back(ChooseAction(percept));
		}
	}

	//ComputeAllPits(percept);

	Action action = actionList.front();
	actionList.pop_front();
	previousAction = action;
	cin.get(); // Pause
	return action;
}

void Agent::GameOver(int score)
{
	firstTry = false;
}

void Agent::UpdateState(Percept &percept)
{
	int orientationInt = (int)worldState.agentOrientation;
	switch (previousAction)
	{
	case GOFORWARD:
		if (percept.Bump)
		{
			// Check if we can determine world size
			if (worldState.agentOrientation == RIGHT)
			{
				worldState.worldSize = worldState.agentLocation.X;
			}
			if (worldState.agentOrientation == UP)
			{
				worldState.worldSize = worldState.agentLocation.Y;
			}
			if (worldState.worldSize > 0)
				FilterSafeLocations();
		}
		else
		{
			Location forwardLocation;
			SetGoForward(forwardLocation);
			worldState.agentLocation = forwardLocation;
			if (worldState.goldLocation == Location(0, 0))
			{
				// If haven't found gold yet, add this location to the pathToGold
				AddToPath(worldState.agentLocation);
			}
		}
		break;

	case TURNLEFT:
		worldState.agentOrientation = (Orientation)((orientationInt + 1) % 4);
		break;

	case TURNRIGHT:
		orientationInt--;
		if (orientationInt < 0)
			orientationInt = 3;
		worldState.agentOrientation = (Orientation)orientationInt;
		break;

	case GRAB:
		worldState.agentHasGold = true;						// Only GRAB when there's gold
		worldState.goldLocation = worldState.agentLocation;
		break;

	case SHOOT:
		worldState.agentHasArrow = false;

		// The only situation where we shoot is if we start out in (1,1)
		// facing RIGHT, perceive a stench, and don't know the wumpus location.
		// In this case, a SCREAM percept means the wumpus is in (2,1) (and dead),
		// or if no SCREAM, then the wumpus is in (1,2) (and still alive).
		possibleWumpusLocations.clear();
		if (percept.Scream)
		{
			possibleWumpusLocations.push_back(Location(2, 1));
		}
		else
		{
			possibleWumpusLocations.push_back(Location(1, 2));
		}

	case CLIMB:
		break;
	}

	// Update visited locations, safe locations, stench locations, clear locations,
	// and possible wumpus locations.
	AddNewLocation(visitedLocations, worldState.agentLocation);
	AddNewLocation(safeLocations, worldState.agentLocation);
	if (percept.Stench)
	{
		AddNewLocation(stenchLocations, worldState.agentLocation);
	}
	else
	{
		AddNewLocation(clearLocations, worldState.agentLocation);
		AddAdjacentLocations(safeLocations, worldState.agentLocation);
	}
	if (possibleWumpusLocations.size() != 1)
	{
		UpdatePossibleWumpusLocations();
	}

	/// Precaution Removal
	list<Location> temp;
	for (Location loc : safeLocations)
	{
		bool is_wumpus = (possibleWumpusLocations.size() == 1) && (loc.X == possibleWumpusLocations.front().X) && (loc.Y == possibleWumpusLocations.front().Y);
		if (prob_pit[loc.X - 1][loc.Y - 1] >= 0.5 || is_wumpus)
		{
			temp.push_back(loc);
		}
	}
	for (Location loc : temp)
	{
		if (prob_pit[loc.X - 1][loc.Y - 1] >= 0.5)
		{
			RemoveLoc(safeLocations, loc);
			cout << "REMOVING [(" << loc.X << ", " << loc.Y << ") = " << prob_pit[loc.X - 1][loc.Y - 1] << "] FROM SAFE_LOC." << endl;
		}
	}

	Output();
}

/*
 * Update possible wumpus locations based on the current set of stench locations
 * and clear locations.
 */
void Agent::UpdatePossibleWumpusLocations()
{
	list<Location> tmpLocations;
	list<Location> adjacentLocations;
	possibleWumpusLocations.clear();
	Location location1, location2;
	list<Location>::iterator itr1, itr2;
	// Determine possible wumpus locations consistent with available stench information
	for (itr1 = stenchLocations.begin(); itr1 != stenchLocations.end(); ++itr1)
	{
		location1 = *itr1;
		// Build list of adjacent locations to this stench location
		adjacentLocations.clear();
		AddAdjacentLocations(adjacentLocations, location1);
		if (possibleWumpusLocations.empty())
		{
			// Must be first stench location in list, so add all adjacent locations
			possibleWumpusLocations = adjacentLocations;
		}
		else
		{
			// Eliminate possible wumpus locations not adjacent to this stench location
			tmpLocations = possibleWumpusLocations;
			possibleWumpusLocations.clear();
			for (itr2 = tmpLocations.begin(); itr2 != tmpLocations.end(); ++itr2)
			{
				location2 = *itr2;
				if (LocationInList(adjacentLocations, location2))
				{
					possibleWumpusLocations.push_back(location2);
				}
			}
		}
	}
	// Eliminate possible wumpus locations adjacent to a clear location
	for (itr1 = clearLocations.begin(); itr1 != clearLocations.end(); ++itr1)
	{
		location1 = *itr1;
		// Build list of adjacent locations to this clear location
		adjacentLocations.clear();
		AddAdjacentLocations(adjacentLocations, location1);
		tmpLocations = possibleWumpusLocations;
		possibleWumpusLocations.clear();
		for (itr2 = tmpLocations.begin(); itr2 != tmpLocations.end(); ++itr2)
		{
			location2 = *itr2;
			if (!LocationInList(adjacentLocations, location2))
			{
				possibleWumpusLocations.push_back(location2);
			}
		}
	}
}

/*
 * Sets the given location to the location resulting in a successful GOFORWARD.
 */
void Agent::SetGoLeft(Location &location)
{
	location = worldState.agentLocation;
	switch (worldState.agentOrientation)
	{
	case RIGHT:
		location.Y++;
		break;
	case UP:
		location.X--;
		break;
	case LEFT:
		location.Y--;
		break;
	case DOWN:
		location.X++;
		break;
	}
}

void Agent::SetGoRight(Location &location)
{
	location = worldState.agentLocation;
	switch (worldState.agentOrientation)
	{
	case RIGHT:
		location.Y--;
		break;
	case UP:
		location.X++;
		break;
	case LEFT:
		location.Y++;
		break;
	case DOWN:
		location.X--;
		break;
	}
}

void Agent::SetGoForward(Location &location)
{
	location = worldState.agentLocation;
	switch (worldState.agentOrientation)
	{
	case RIGHT:
		location.X++;
		break;
	case UP:
		location.Y++;
		break;
	case LEFT:
		location.X--;
		break;
	case DOWN:
		location.Y--;
		break;
	}
}

/*
 * Add given location to agent's pathToGold. But if this location is already on the
 * pathToGold, then remove everything after the first occurrence of this location.
 */
void Agent::AddToPath(Location &location)
{
	list<Location>::iterator itr = find(pathToGold.begin(), pathToGold.end(), location);
	if (itr != pathToGold.end())
	{
		// Location already on path (i.e., a loop), so remove everything after this element
		++itr;
		pathToGold.erase(itr, pathToGold.end());
	}
	else
	{
		pathToGold.push_back(location);
	}
}

/*
 * Choose and return an action when we haven't found the gold yet.
 * Handle special case where we start out in (1,1), perceive a stench, and
 * don't know the wumpus location. In this case, SHOOT. Orientation will be
 * RIGHT, so if SCREAM, then wumpus in (2,1); otherwise in (1,2). This is
 * handled in the UpdateState method.
 */
Action Agent::ChooseAction(Percept &percept)
{
	Action action;
	Location forwardLocation;
	SetGoForward(forwardLocation);
	if (percept.Stench && (worldState.agentLocation == Location(1, 1)) &&
		(possibleWumpusLocations.size() != 1))
	{
		action = SHOOT;
	}
	else if (LocationInList(safeLocations, forwardLocation) &&
			 (!LocationInList(visitedLocations, forwardLocation)))
	{
		// If happen to be facing safe unvisited location, then move there
		action = GOFORWARD;
	}
	else
	{
		list<Location> turn_list;
		Location l_loc, r_loc;
		SetGoLeft(l_loc);
		SetGoRight(r_loc);

		//cout << "-------- L_TURN = " << get_pit_prob(l_loc) << endl;
		//cout << "-------- R_TURN = " << get_pit_prob(r_loc) << endl;

		bool is_safe = LocationInList(possibleWumpusLocations, forwardLocation) ||
					   OutsideWorld(forwardLocation) || LocationInList(knownPitLoc, forwardLocation);
		if (get_pit_prob(l_loc) < get_pit_prob(r_loc) && is_safe)
		{
			actionList.push_back(TURNLEFT);
			actionList.push_back(GOFORWARD);
			//action = TURNLEFT;
		}
		else if (get_pit_prob(l_loc) > get_pit_prob(r_loc) && is_safe)
		{
			actionList.push_back(TURNRIGHT);
			actionList.push_back(GOFORWARD);
			//action = TURNRIGHT;
		}
		else
		{ // Both values are same, turn around
			if (get_pit_prob(l_loc) < 0.5)
			{
				action = TURNLEFT;
			}
			else
			{
				actionList.push_back(TURNRIGHT);
				actionList.push_back(TURNRIGHT);
				actionList.push_back(GOFORWARD);
			}
		}

		/*else
		{
			// Choose randomly from GOFORWARD, TURNLEFT, and TURNRIGHT, but don't
			// GOFORWARD into a possible wumpus location or a wall
			if (is_safe))
			{
				action = (Action)((rand() % 2) + 1); // TURNLEFT, TURNRIGHT
			}
			else
			{
				//action = (Action)(rand() % 3); // GOFORWARD, TURNLEFT, TURNRIGHT
			}
		}*/
	}
	return action;
}

float Agent::get_pit_prob(Location &loc)
{
	if (OutsideWorld(loc))
	{
		return 2.00; // Error message to know if out of bounds
	}
	return prob_pit[loc.X - 1][loc.Y - 1];
}

/*
 * Add a sequence of actions to actionList that moves the agent along the
 * pathToGold if forward=true, or the reverse if forward=false. Assumes at
 * least one location is on pathToGold.
 */
void Agent::AddActionsFromPath(bool forward)
{
	list<Location> path = pathToGold;
	if (!forward)
		path.reverse();
	Location currentLocation = worldState.agentLocation;
	Orientation currentOrientation = worldState.agentOrientation;
	list<Location>::iterator itr_loc = path.begin();
	++itr_loc;
	while (itr_loc != path.end())
	{
		Location nextLocation = *itr_loc;
		Orientation nextOrientation;
		if (nextLocation.X > currentLocation.X)
			nextOrientation = RIGHT;
		if (nextLocation.X < currentLocation.X)
			nextOrientation = LEFT;
		if (nextLocation.Y > currentLocation.Y)
			nextOrientation = UP;
		if (nextLocation.Y < currentLocation.Y)
			nextOrientation = DOWN;
		// Find shortest turn sequence (assuming RIGHT=0, UP=1, LEFT=2, DOWN=3)
		int diff = ((int)currentOrientation) - ((int)nextOrientation);
		if ((diff == 1) || (diff == -3))
		{
			actionList.push_back(TURNRIGHT);
		}
		else
		{
			if (diff != 0)
				actionList.push_back(TURNLEFT);
			if ((diff == 2) || (diff == -2))
				actionList.push_back(TURNLEFT);
		}
		actionList.push_back(GOFORWARD);
		currentLocation = nextLocation;
		currentOrientation = nextOrientation;
		++itr_loc;
	}
}

/*
 * Return true if given location is in given location list; otherwise, return false.
 */
bool Agent::LocationInList(list<Location> &locationList, const Location &location)
{
	if (find(locationList.begin(), locationList.end(), location) != locationList.end())
	{
		return true;
	}
	return false;
}

/*
 * Add location to given list, if not already present.
 */
void Agent::AddNewLocation(list<Location> &locationList, const Location &location)
{
	if (!LocationInList(locationList, location))
	{
		locationList.push_back(location);
	}
}

/*
 * Add locations that are adjacent to the given location to the given location list.
 * Doesn't add locations outside the left and bottom borders of the world, but might
 * add locations outside the top and right borders of the world, if we don't know the
 * world size.
 */
void Agent::AddAdjacentLocations(list<Location> &locationList, const Location &location)
{
	int worldSize = worldState.worldSize;
	if ((worldSize == 0) || (location.Y < worldSize))
	{
		AddNewLocation(locationList, Location(location.X, location.Y + 1)); // up
	}
	if ((worldSize == 0) || (location.X < worldSize))
	{
		AddNewLocation(locationList, Location(location.X + 1, location.Y)); // right
	}
	if (location.X > 1)
		AddNewLocation(locationList, Location(location.X - 1, location.Y)); // left
	if (location.Y > 1)
		AddNewLocation(locationList, Location(location.X, location.Y - 1)); // down
}

/*
 * Return true if given location is outside known world.
 */
bool Agent::OutsideWorld(Location &location)
{
	int worldSize = worldState.worldSize;
	if ((location.X < 1) || (location.Y < 1))
		return true;
	if ((worldSize > 0) && ((location.X > worldSize) || (location.Y > worldSize)))
		return true;
	return false;
}

/*
 * Filters from safeLocations any locations that are outside the upper or right borders of the world.
 */
void Agent::FilterSafeLocations()
{
	int worldSize = worldState.worldSize;
	list<Location> tmpLocations = safeLocations;
	safeLocations.clear();
	for (list<Location>::iterator itr = tmpLocations.begin(); itr != tmpLocations.end(); ++itr)
	{
		Location location = *itr;
		if ((location.X < 1) || (location.Y < 1))
			continue;
		if ((worldSize > 0) && ((location.X > worldSize) || (location.Y > worldSize)))
		{
			continue;
		}
		safeLocations.push_back(location);
	}
}

void Agent::Output()
{
	list<Location>::iterator itr;
	Location location;
	cout << "World Size: " << worldState.worldSize << endl;
	cout << "Visited Locations:";
	for (itr = visitedLocations.begin(); itr != visitedLocations.end(); ++itr)
	{
		location = *itr;
		cout << " (" << location.X << "," << location.Y << ")";
	}
	cout << endl;
	cout << "Safe Locations:";
	for (itr = safeLocations.begin(); itr != safeLocations.end(); ++itr)
	{
		location = *itr;
		cout << " (" << location.X << "," << location.Y << ")";
	}
	cout << endl;
	cout << "Possible Wumpus Locations:";
	for (itr = possibleWumpusLocations.begin(); itr != possibleWumpusLocations.end(); ++itr)
	{
		location = *itr;
		cout << " (" << location.X << "," << location.Y << ")";
	}
	cout << endl;
	cout << "Gold Location: (" << worldState.goldLocation.X << "," << worldState.goldLocation.Y << ")\n";
	cout << "Path To Gold:";
	for (itr = pathToGold.begin(); itr != pathToGold.end(); ++itr)
	{
		location = *itr;
		cout << " (" << location.X << "," << location.Y << ")";
	}
	cout << endl;
	cout << "Action List:";
	for (list<Action>::iterator itr_a = actionList.begin(); itr_a != actionList.end(); ++itr_a)
	{
		cout << " ";
		PrintAction(*itr_a);
	}
	cout << endl;

	cout << endl;
}

// ----

/*foreach location (x,y) in frontier
{
	P(Pitx,y=true) = 0.0
	P(Pitx,y=false) = 0.0
	frontier’ = frontier – {Pitx,y}
	foreach possible combination C of pit=true and pit=false in frontier’
	{
		P(frontier’) = (0.2)T * (0.8)F, where T = number of pit=true in C, and
		F = number of pit=false in C
		if breeze is consistent with (C + Pitx,y=true)
		then P(Pitx,y=true) += P(frontier’)
		if breeze is consistent with (C + Pitx,y=false)
		then P(Pitx,y=false) += P(frontier’)
	}
	P(Pitx,y=true) *= 0.2
	P(Pitx,y=false) *= 0.8
	P(Pitx,y=true) = P(Pitx,y=true) / ( P(Pitx,y=true) + P(Pitx,y=false) ) // normalize
}*/

void Agent::ComputeAllPits(Percept &percept)
{ // CP
	list<Location>::iterator itr, itr_f1, itr_f2;
	Location loc;

	/// All KNOWN Loc is 0.00
	for (itr = visitedLocations.begin(); itr != visitedLocations.end(); ++itr)
	{
		loc = *itr;
		Pit(loc.X, loc.Y, 0.00);
		RemoveLoc(frontier, loc);
	}
	if (possibleWumpusLocations.size() == 1)
	{
		loc = *visitedLocations.begin();
		Pit(loc.X, loc.Y, 0.00);
	}

	/// Find Frontier Nodes
	list<Location> adj_loc = FindFrontier();

	if (knownPitLoc.size() != 0)
	{
		AdjList(knownPitLoc, worldState.agentLocation);
		/// TO-DO: Make
	}

	if (percept.Breeze && !AdjList(knownPitLoc, worldState.agentLocation))
	{
		/// Adj. are possible pits
		for (Location loc : adj_loc)
		{
			if (!SearchListLoc(possiblePitLoc, loc) && prob_pit[loc.X - 1][loc.Y - 1] != 0 && prob_pit[loc.X - 1][loc.Y - 1] != 1)
				possiblePitLoc.push_back(loc);
		}

		ComputeFrontierSum();
	}
	else if (!percept.Breeze && !AdjList(knownPitLoc, worldState.agentLocation))
	{
		for (itr = adj_loc.begin(); itr != adj_loc.end(); ++itr)
		{
			loc = *itr;
			cout << "\n[~] COMPUTING Prob_Pit(" << loc.X << ", " << loc.Y << "):\n";
			if (get_pit_prob(loc) != 1.00)
			{
				Pit(loc.X, loc.Y, 0.00);
			}
			cout << "\tprob_Pit(" << loc.X - 1 << ", " << loc.Y - 1 << ") = " << prob_pit[loc.X - 1][loc.Y - 1];
			RemoveLoc(possiblePitLoc, loc);
		}
		cout << endl;

		if (possiblePitLoc.size() == 1 && !SearchListLoc(breezeList, possiblePitLoc.front()))
		{ // Narrowed down which is Pit
			prob_pit[possiblePitLoc.front().X - 1][possiblePitLoc.front().Y - 1] = 1.00;
			pitList.push_back(possiblePitLoc.front());
			possiblePitLoc.pop_back();
		}
	}
	else
	{
	}
	list<Location> temp;
	for (Location loc : possiblePitLoc)
	{
		if (prob_pit[loc.X - 1][loc.Y - 1] == 0.00 || prob_pit[loc.X - 1][loc.Y - 1] == 1.00)
		{
			temp.push_back(loc);
		}
	}
	for (Location loc : temp)
	{
		RemoveLoc(possiblePitLoc, loc);
	}

	if (possiblePitLoc.size() == 1 && worldState.agentAlive)
	{
		loc = *possiblePitLoc.begin();
		Pit(loc.X, loc.Y, 1.00);
		knownPitLoc.push_back(loc);
		possiblePitLoc.pop_back();
	}

	/*cout << "\nPOSSIBLE PIT LOC:";
	for (Location loc : possiblePitLoc)
		cout << "(" << loc.X << ", " << loc.Y << ") ";
	cout << endl;*/

	/// Remove (<= 0.5) from SafeLocations
	for (Location loc : safeLocations)
	{
		bool is_wumpus = (possibleWumpusLocations.size() == 1) && (loc.X == possibleWumpusLocations.front().X) && (loc.Y == possibleWumpusLocations.front().Y);
		if (prob_pit[loc.X - 1][loc.Y - 1] >= 0.5 || is_wumpus)
		{
			temp.push_back(loc);
		}
	}
	for (Location loc : temp)
	{
		if (prob_pit[loc.X - 1][loc.Y - 1] >= 0.5)
		{
			RemoveLoc(safeLocations, loc);
			cout << "REMOVING [(" << loc.X << ", " << loc.Y << ") = " << prob_pit[loc.X - 1][loc.Y - 1] << "] FROM SAFE_LOC." << endl;
		}
	}

	PrintProbPit();
}

float Agent::ComputeFrontierSum()
{
	list<Location>::iterator itr, itr_f1, itr_f2;
	list<Location> adj_frontier;

	if (!SearchListLoc(breezeList, worldState.agentLocation))
		breezeList.push_back(worldState.agentLocation);

	/// alpha P(p_x,y) SUM[Frontier] P(breeze|known, p_x,y, Frontier) P(Frontier)
	/// True = 0.2 / False = 0.8

	list<Location> frontier_temp;
	float sum_frontier_t = 0.00, sum_frontier_f = 0.00;
	float pit_t = 0.00, pit_f = 0.00;

	for (itr = frontier.begin(); itr != frontier.end(); ++itr)
	{
		Location loc_f1 = *itr;

		cout << "[~] COMPUTING Prob_Pit(" << loc_f1.X << ", " << loc_f1.Y << "):\n";

		frontier_temp.clear();
		list<bool> adj_to_breeze;

		cout << "\tfrontier' =";

		for (Location loc_f2 : frontier)
		{ /// 1.) Creates Frontier' ["frontier_temp"]
			if (!(loc_f2.X == loc_f1.X && loc_f2.Y == loc_f1.Y))
			{ /// Exclude current Frontier node
				frontier_temp.push_back(loc_f2);
				cout << " (" << loc_f2.X << ", " << loc_f2.Y << ")";
			}
		}
		cout << endl;

		/// 2.) Iterate Frontier' -> Compare pits TRUTH Tables
		int f_size = frontier_temp.size();
		//list<bool> truth_table;
		if (!(prob_pit[loc_f1.X][loc_f1.Y] == 1.00 || prob_pit[loc_f1.X][loc_f1.Y] == 0.00))
		{

			sum_frontier_t = FindTruthTable(f_size, frontier_temp, loc_f1, true);
			sum_frontier_f = FindTruthTable(f_size, frontier_temp, loc_f1, false);

			//cout << "-- SUM_FRONTIER_T = " << sum_frontier_t << endl;
			//cout << "-- SUM_FRONTIER_F = " << sum_frontier_f << endl;

			pit_t = (0.2) * sum_frontier_t;
			pit_f = (0.8) * sum_frontier_f;

			//cout << "-- PIT_T = " << pit_t << endl;
			//cout << "-- PIT_F = " << pit_f << endl;

			/// Normalize
			float normalize = 1 / (pit_t + pit_f);
			pit_t *= normalize;
			pit_f *= normalize;

			//cout << "[%]possiblePitLoc SIZE = " << possiblePitLoc.size() << endl;
			list<Location> adj_loc;
			bool visited_pit = false;

			AddAdjacentLocations(adj_loc, worldState.agentLocation);
			for (Location loc : adj_loc)
			{
				if (prob_pit[loc.X - 1][loc.Y - 1] == 1.00)
				{
					//possiblePitLoc.pop_back();
					visited_pit = true;
					possiblePitLoc.clear();
				}
			}
			bool is_pit = possiblePitLoc.size() == 1 && loc_f1.X == possiblePitLoc.front().X && loc_f1.Y == possiblePitLoc.front().Y;
			if (is_pit && !visited_pit && !SearchListLoc(breezeList, possiblePitLoc.front()))
			{ // Narrowed down which is Pit
				prob_pit[loc_f1.X - 1][loc_f1.Y - 1] = 1.00;
				possiblePitLoc.pop_back();
			}
			else if ((prob_pit[loc_f1.X - 1][loc_f1.Y - 1] != 0) && (prob_pit[loc_f1.X - 1][loc_f1.Y - 1] != 1) && !visited_pit)
			{
				if (get_pit_prob(loc_f1) == 0.00 || get_pit_prob(loc_f1) == 1.00)
				{
					pit_t = 0.00;
				}
				prob_pit[loc_f1.X - 1][loc_f1.Y - 1] = pit_t;
			}
		}
		cout << "\tprob_Pit(" << loc_f1.X << ", " << loc_f1.Y << "): " << pit_t << endl;
	}

	/// 3.) Normalize

	/// NEXT STEP: Make Agent move to areas with smallest percentage
}

float Agent::FindTruthTable(int n, list<Location> &L, Location &pit_x_y, bool is_xy_pit)
{ /// Note: 0 = T, 1 = F
	list<Location>::iterator itr;
	vector<vector<int>> output(n, vector<int>(1 << n));
	float prob[2] = {0.2, 0.8};
	float f_product, f_sum = 0;
	bool B_FLAG;

	unsigned result = 1U << (n - 1);

	/// Creates Truth Table
	for (unsigned col = 0; col < n; ++col, result >>= 1U)
	{
		for (unsigned row = result; row < (1U << n); row += (result * 2))
		{
			fill_n(&output[col][row], result, 1);
		}
	}

	// These loops just print out the results, nothing more.
	for (unsigned x = 0; x < (1 << n); ++x)
	{
		//cout << endl;
		if (is_xy_pit == true)
		{
			B_FLAG = AdjBreeze(pit_x_y);
			//cout << "[!!!!!!!!] AdjBreeze(" << pit_x_y.X << ", " << pit_x_y.Y << "): "<< B_FLAG << " -- \n";
		}
		else
		{
			B_FLAG = false;
		}

		itr = L.begin();
		f_product = 1;
		for (unsigned y = 0; y < n; ++y)
		{
			/// Respective Location
			Location loc_f1 = *itr;
			//cout << "(" << loc_f1.X << ", " << loc_f1.Y << "):";	// debug (To view Truth Tables)
			// cout << " " << output[y][x] << " | ";

			/// Location Truth Value
			f_product *= prob[output[y][x]];

			/// 1.) Compare all nodes with 0 to B, if any are ADJ -> Breeze Flag == TRUE
			if ((output[y][x] == 0) && AdjBreeze(loc_f1))
			{
				B_FLAG = true;
			}
			//cout << B_FLAG;
			//cout << "[RESULT: " << (B_FLAG == true) << "" << is_xy_pit << "] ";
			/// 2.) if Breeze Flag is on, MULT all values together. Else, return 0
			if ((y == n - 1) && (B_FLAG == true))
			{
				f_sum += f_product;
				//cout << "... B_FLAG = " << B_FLAG << ", F_SUM = " << f_sum;	// debug
			}

			++itr;
		}
		//cout << "--------- SUM OF ALL: " << f_sum;
		//cout << endl;
	}
	return f_sum;
	/// TO-DO: Assign Y1 -> F1, Y2 -> F2
}

bool Agent::AdjBreeze(Location &loc)
{
	for (Location b : breezeList)
		if (Adjacent(b, loc))
			return true;
	return false;
}

bool Agent::AdjList(list<Location> &L, Location &loc)
{
	for (Location l : L)
		if (Adjacent(l, loc))
			return true;
	return false;
}

list<Location> Agent::FindFrontier()
{
	list<Location>::iterator itr;
	Location loc;

	/// Calculate Frontier Nodes (is in safeLocations)
	list<Location> adj_loc;
	adj_loc.clear();
	AddAdjacentLocations(adj_loc, worldState.agentLocation);

	/*cout << "---- FRONTIER (Before): ";
	for (Location loc : adj_loc)
		cout << "(" << loc.X << ", " << loc.Y << ") ";
	cout << endl;*/

	/// Find Frontier

	for (itr = adj_loc.begin(); itr != adj_loc.end(); ++itr)
	{
		loc = *itr;

		/// Add to Frontier
		if (prob_pit[loc.X - 1][loc.Y - 1] != 0.00 && !SearchListLoc(frontier, loc) && !SearchListLoc(visitedLocations, loc))
		{
			frontier.push_back(loc);
		}
	}

	cout << "[%] FRONTIER: ";
	for (Location loc : frontier)
		cout << "(" << loc.X << ", " << loc.Y << ") ";
	cout << "\n";

	return adj_loc;
}

bool Agent::RemoveLoc(list<Location> &list_loc, Location &loc)
{
	list<Location>::iterator itr;

	for (itr = list_loc.begin(); itr != list_loc.end(); ++itr)
	{
		Location L = *itr;
		if ((L.X == loc.X) && (L.Y == loc.Y))
		{
			//cout << "[!] - RemoveLoc() = TRUE / Removed (" << loc.X << ", " << loc.Y << ").\n";	// debug
			list_loc.erase(itr);
			return true;
		}
	}
	//cout << "[!] - RemoveLoc() = FALSE\n";	// debug
	return false;
}

void Agent::PrintProbPit()
{
	cout << "\nP(Pit):\n";
	for (int y = 4; y >= 0; --y)
	{
		for (int x = 0; x < 5; x++)
		{
			if (x + 1 == worldState.agentLocation.X && y + 1 == worldState.agentLocation.Y) // debug (To see current location)
				cout << "*";
			else if ((possibleWumpusLocations.size() == 1) && (x == possibleWumpusLocations.front().X - 1) && (y == possibleWumpusLocations.front().Y - 1))
				cout << "w";
			else
				cout << " ";

			cout << prob_pit[x][y] << " ";
			//cout << "PROB:PIT(" << x << ", " << y << ") = " << prob_pit[x ][y ] << endl;	//debug
		}
		cout << endl;
	}
	cout << endl;
}

void Agent::Pit(int x, int y, float v)
{
	prob_pit[x - 1][y - 1] = v;
	//cout << "PROB:PIT(" << x - 1 << ", " << y - 1 << ") = " << prob_pit[x - 1][y - 1] << endl;
}

bool Agent::SearchListLoc(list<Location> &list_loc, Location &loc)
{
	list<Location>::iterator itr;

	for (itr = list_loc.begin(); itr != list_loc.end(); ++itr)
	{
		Location L = *itr;
		if ((L.X == loc.X) && (L.Y == loc.Y))
		{
			//cout << "[!] - SearchListLoc() = TRUE / (" << loc.X << "," << loc.Y << ")\n";	// debug
			return true;
		}
	}
	//cout << "[!] - SearchListLoc() = FALSE / (" << loc.X << "," << loc.Y << ")\n";	// debug
	return false;
}

void Agent::CheckFrontier(int x, int y)
{
	float y_true = 0.0, y_false = 0.0;
	float new_frontier;
	// P(Pitx,y=true) = 0.0
	// P(Pitx,y=false) = 0.0
	// frontier’ = frontier – {Pitx,y}
}
