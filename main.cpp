#define EMPTY 0
#define TRAP 1
#define OBJECTIVE 2
#define BONUS 3
#define INIT 4
  
#define TRAP_SCORE -100
#define OBJECTIVE_SCORE 100
#define EMPTY_SCORE 0
#define BONUS_SCORE 1
#define INIT_SCORE 0

#define MAP_FILE "map.csv"

// Actions
#define UP 0
#define DOWN 1
#define RIGHT 2
#define LEFT 3
#define NUM_ACTIONS 4

// Reinforcement learning constants
#define EPSILON 0.9          // Posibility of choosing qTable strategy (vs random)
#define ALPHA 0.1            // Learning rate
#define GAMMA 0.9            // Discount
#define MAX_EPISODES 10000

#define REFRESH_TIME 100000
#define ENABLE_VISUALIZATION true

// Debug disables random seed and enables more detailed logs
#define DEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

int randomStrategy(int n);
double maxItem(double* array, int N_ITEMS);
int getStateScore(int state);
double rand_0_1(void);
int qTableStrategy(int posX, int posY, double *** qTable);
int maxIndex(double* array, int N_ITEMS);
void printMap(int ** map, int mapSizeX, int mapSizeY, int posX, int posY, int step) ;
int** allocMap(int sizeX, int sizeY);
void copyMap(int** map1, int**map2, int sizeX, int sizeY);
void readMapToMemory(FILE* mapFile, int** map, int mapSizeX, int mapSizeY, int* originX, int* originY);
double*** generateQTable(int mapSizeX, int mapSizeY);

/**
 * The ACTOR should reach the objective, gaining the most points he can and avoiding the traps.
 * Basically the ACTOR should maximize the score.
 * This program uses reinforcement learning with the q-learning concept.
 */
int main() 
{
  int posX, posY;

  if (!DEBUG) {
      srand(time(NULL));
  }  

  // Getting map data from file
  FILE* mapFile = fopen(MAP_FILE,"r");
  char charOfFile;
  int mapSizeX = 0;
  int mapSizeY = 1;

  // Getting map size
  while((charOfFile = fgetc(mapFile)) != EOF) {
    if (mapSizeY == 1 && charOfFile != ',' && charOfFile != '\n') {
      mapSizeX++;
    }

    if (charOfFile == '\n') {
      mapSizeY++;
    }
  }

  if (DEBUG) {      
      printf("Creating map of %d x %d\n", mapSizeX, mapSizeY);
  }

  int **map = allocMap(mapSizeX, mapSizeY);
  int **originalMap = allocMap(mapSizeX, mapSizeY);

  int originX = 0;
  int originY = 0;

  readMapToMemory(mapFile, originalMap, mapSizeX, mapSizeY, &originX, &originY);

  fclose(mapFile);
  // End of reading map data from file

  // qTable memory allocation (more fun!)
  double *** qTable = generateQTable(mapSizeX, mapSizeY);
  
  int episodes = 0;
  int steps;
  int score;

  for(int i = 0; i < MAX_EPISODES; i++) {
    score = 0;
    steps = 0;

    posX = originX;
    posY = originY;

    copyMap(map, originalMap, mapSizeX, mapSizeY);    

    int state = map[posX][posY];    
  
    // While I don't reach the obective
    while (state != OBJECTIVE) {        
      // We show last episodes
      if (i == MAX_EPISODES - 1 && ENABLE_VISUALIZATION) {        
        printMap(map, mapSizeX, mapSizeY, posX, posY, steps);
      }      
      
      int accion = qTableStrategy(posX, posY, qTable);

      int oldPosX, oldPosY;

      oldPosX = posX;
      oldPosY = posY;
      
      state = map[posX][posY];
      
      // qPredict is what we predict that the current state + action will produce
      int qPredict = qTable[posX][posY][accion];      
    
      switch(accion) {
      case UP:
        if (posY > 0) {
          posY--;        
        }      
        break;
      case DOWN:
        if (posY < mapSizeY - 1) {
          posY++;        
        }      
        break;

      case LEFT:
        if (posX > 0) {
          posX--;        
        }      
        break;

      case RIGHT:
        if (posX < mapSizeX - 1) {
          posX++;
        }      
        break;      
      }

      int newState = map[posX][posY];
      
      int qTarget;

      int stateScore = getStateScore(newState);

      if (newState == OBJECTIVE) {
        qTarget = stateScore;
      } else {
        qTarget = stateScore + GAMMA * maxItem(qTable[posX][posY], NUM_ACTIONS);        
      }
      
      // We update qTable with new information about the environment
      qTable[oldPosX][oldPosY][accion] += ALPHA * (qTarget - qPredict);
      
      score += stateScore;
      
      // If player took bonus, we remove the bonus from the map
      // (otherwise the program might be optimized, depending on the values,
      // for a infinite loop)
      if (newState == BONUS) {
        map[posX][posY] = EMPTY;
      }      
      
      steps++;      
    }

    if (DEBUG) {
        printf("Episode %d\n", i+1);
        printf("Score: %d\n", score);
        printf("Steps: %d\n", steps);
        printf("------------\n");
    }
  }

  // In case of moving main() to another function, here is where we need to free memory from qTable and maps

  printf("Last episode data: \n");
  printf("Score: %d\n", score);
  printf("Steps: %d\n", steps);

  return 0;  
}

/**
 * Clears the screen and prints the map in it. Useful for seeing in a visual way 
 * what the agent is doing.
 */
void printMap(int ** map, int mapSizeX, int mapSizeY, int posX, int posY, int step) 
{
  system("clear");

  for(int i = 0; i < mapSizeX; i++) {
    for (int j = 0; j < mapSizeY; j++) {
      if (i == posX && j == posY) {
        printf("A");
      } else {
        switch(map[i][j]) {
        case EMPTY:
          printf(" ");
          break;
        case TRAP:
          printf("X");
          break;
        case BONUS:
          printf("B");
          break;
        case OBJECTIVE:
          printf("O");
          break;
        case INIT:
          printf("I");
          break;            
        }

      }
      printf("|");      
      
    }
    printf("\n");    
  }

  printf("Step: %d\n", step);  

  usleep(REFRESH_TIME);  
}

/**
 * Returns a random int between 0 and n
 */
int randomStrategy(int n) 
{
  return rand() % n;  
}

/**
 * Makes a decision about what to do.
 */
int qTableStrategy(int posX, int posY, double *** qTable) 
{
  // This is the way of choosing the action
  double* stateActions = qTable[posX][posY];

  double max = maxItem(stateActions, NUM_ACTIONS);

  double rand = rand_0_1();
  
  if (max == 0 || rand > EPSILON) {
    return randomStrategy(NUM_ACTIONS);
  } else {
    return maxIndex(stateActions, NUM_ACTIONS);
  }
}

/**
 * Returns the index with the max value of an int list
 */
int maxIndex(double* array, int N_ITEMS) 
{  
  int index = 0;
  
  for(int i = 0; i < N_ITEMS; i++) {
    if (array[i] > array[index]) {
      index = i;      
    }
  }
  
  return index;   
}

/**
 * Returns the max value of an int list
 */
double maxItem(double* array, int N_ITEMS) 
{
  double max = array[0];

  for(int i = 0; i < N_ITEMS; i++) {
    if (array[i] > max) {
      max = array[i];      
    }
  }
  
  return max;  
}

/**
 * Returns the score defined for each state.
 */
int getStateScore(int state) 
{
  int score = 0;
  
  switch(state) {
  case EMPTY:
    score = EMPTY_SCORE;
    break;
  case TRAP:
    score = TRAP_SCORE;
    break;
  case OBJECTIVE:
    score = OBJECTIVE_SCORE;
    break;
  case BONUS:
    score = BONUS_SCORE;
    break;
  case INIT:
    score = INIT_SCORE;
    break;
  }
  
  return score;  
}

/**
 * Returns a float random between 0 and 1
 */
double rand_0_1(void)
{
  return rand() / ((double) RAND_MAX);
}

/**
 * Reserves memory for a map
 */
int** allocMap(int mapSizeX, int mapSizeY) 
{
  // First we reserve memory for the pointers array
  int** map = (int**) malloc(sizeof(int*) * mapSizeX);

  if(map == NULL)           
  {
    printf("Error! memory of map not allocated.");
    exit(0);
  }
  // Then we reserve space for all rows in the matrix
  for(int i = 0; i < mapSizeX; i++) {
    map[i] = (int*) malloc(sizeof(int) * mapSizeY);

    if (map[i] == NULL) {
      printf("Unable to allocate memory");
      exit(0);
    }    
  }

  return map;  
}

/**
 * Copies all values from map2 to map1
 */
void copyMap(int** map1, int**map2, int mapSizeX, int mapSizeY) 
{
    for (int i = 0; i < mapSizeX; i++) {
        for (int j = 0; j < mapSizeY; j++) {
            map1[i][j] = map2[i][j];            
        }     
    }
}

/**
 * Reads a map of size mapSizeX and mapSizeY from file and put it into memory.
 * It also sets variables originX and originY
 */
void readMapToMemory(FILE* mapFile, int** map, int mapSizeX, int mapSizeY, int * originX, int * originY) 
{
  // Setting values as zero just in case the init is not present in the map
  (*originX) = 0;
  (*originY) = 0;
    
  rewind(mapFile);

  // Reading map from map file
  char charOfFile = fgetc(mapFile); 
    
  for (int i = 0; i < mapSizeX; i++) {
    for (int j = 0; j < mapSizeY; j++) {          
      while(charOfFile == ',' || charOfFile == '\n') {
        charOfFile = fgetc(mapFile);
      }

      map[i][j] = atoi(&charOfFile);

      // 4 is the init position
      if (charOfFile == '4') {
        (*originX) = i;
        (*originY) = j;
      }
     
      charOfFile = fgetc(mapFile);
    }     
  }
}

/**
 * Allocates memory for qtable and initialize its values (with zeroes)
 */
double*** generateQTable(int mapSizeX, int mapSizeY) 
{
  double*** qTable = (double***) malloc(sizeof(double*) * mapSizeX);

  if (qTable == NULL) {
      printf("Unable to allocate memory");
      exit(0);
  }  

  for(int i = 0; i < mapSizeX; i++) {
    qTable[i] = (double**) malloc(sizeof(double) * mapSizeY);

    if (qTable[i] == NULL) {
      printf("Unable to allocate memory");
      exit(0);
    }

    for (int j = 0; j < mapSizeY; j++) {
      qTable[i][j] = (double*) malloc(sizeof(double) * NUM_ACTIONS);

      if (qTable[i][j] == NULL) {
          printf("Unable to allocate memory");
          exit(0);
      }  
    }    
  }
  
  // Initializing qTable with zeroes
  for(int i = 0; i < mapSizeX; i++) {
    for (int j = 0; j < mapSizeY; j++) {
      for(int k = 0; k < NUM_ACTIONS; k++) {        
        qTable[i][j][k] = 0;
      }      
    }    
  }

  return qTable;  
}
