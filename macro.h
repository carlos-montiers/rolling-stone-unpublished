int     GrowRoom1(MAZE *maze, PHYSID pos, PHYSID prev, GROOM *groom);
int     GrowRoom2(MAZE *maze, PHYSID pos, PHYSID prev, GROOM *groom);
int     GrowRoom(MAZE *maze, PHYSID pos, GROOM *groom);
void    PickUpEntrances(MAZE *maze, int gridx);
int     SubMacro(MAZE *maze, MOVE *moves, int *move_number);
void    RemoveGRoom(MAZE *maze, int gridx);
void    FindMacros(MAZE *maze);

PHYSID  FindEndTunnel(MAZE *maze, PHYSID pos, int dir, PHYSID *last_over);
void    AddMacro(MAZE *maze, PHYSID pos, int type, PHYSID from, 
			    PHYSID last_over, PHYSID to);

int 	StartBuildGMTree(MAZE *start_maze, GROOM *groom);
GMNODE *BuildGMTree(int depth, GROOM *groom);
void    DelGMTree(GMNODE *gmnode);
void    GoalReach(MAZE *maze, BitString v, PHYSID start, 
		  PHYSID manpos, int index, int optimal);
int     DeadGoal(MAZE *maze, PHYSID pos);

void InitGRoom(GROOM *groom, int gridx);
void GroomExcPos(MAZE *maze, PHYSID pos, GROOM *groom);
void GroomIncPos(MAZE *maze, PHYSID pos, GROOM *groom);
int  EvaluateGroom(GROOM *groom);
void AsimGoals(MAZE *maze, PHYSID pos, GROOM *groom);
void GrowDFS(MAZE *maze, GROOM *groom, int g);
int  GrowRoom3(MAZE *maze, PHYSID pos, PHYSID prev, GROOM *groom);
