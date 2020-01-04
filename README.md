# Wumpus World Simulator

Simulator Options
-----------------

The wumpus simulator takes a few options, as described below.

`-size <N>` lets you to set the size of the world to NxN (N>1). Default is 4.

`-trials <N>` runs the simulator for N trials, where each trial generates a new
wumpus world. Default is 1.

`-tries <N>` runs each trial of the simulator N times, giving your agent
multiple tries at the same world. Default is 1.

`-seed <N>` lets you set the random number seed to some positive integer so that
the simulator generates the same sequence of worlds each run. By default, the
random number seed is set semi-randomly based on the clock.

`-world <file>` lets you play a specific world as specified in the given file.
The -size option is ignored, and each try and trial uses the same world. The
format of the world file is as follows (all lowercase, must appear in this
order):

```
size N
wumpus N N
gold N N
pit N N
pit N N
...

```
where N is a positive integer. Some error checking is performed. A sample
world file is provided in testworld.txt.

## Simulator Details

The simulator works by generating a new world and a new agent for each trial.
Before each try on this world, the agent's Initialize() method is called, which
you can use to perform any pre-game preparation. Then, the game starts.  The
agent's Process() method is called with the current Percept, and the agent
should return an action, which is performed in the simulator. This continues
until the game is over (agent dies or leaves cave) or the maximum number of
moves (1000) is exceeded. When the game is over, the Agent's GameOver() method
is called. If additional tries are left for this world, then the world is
re-initialized, and the agent's Initialize() method is called again, and play
proceeds on another instance of the same game.

After the number of tries is completed, the agent is deleted. So, you may want
to store some information in the agent's destructor method to be reloaded
during the agent's constructor method when reborn for the next trial. If
additional trials have been requested, then a new wumpus world is generated,
and the process continues as described above.

Scoring information is output at the end of each try, each trial and at the end
of the simulation.
