/**
 * File: life.cpp
 * --------------
 * Implements the Game of Life.
 */

#include <iostream>  // for cout
#include <sstream>   // for I/O
#include <iomanip>   // for manipulators
using namespace std;

#include "console.h" // required of all files that contain the main function
#include "simpio.h"  // for getLine
#include "gevents.h" // for mouse event detection
#include "strlib.h"
#include "filelib.h" // for I/O files
#include "random.h"  // for random grid

#include "life-constants.h"  // for kMaxAge
#include "life-graphics.h"   // for class LifeDisplay

// Constants
static const string userAffirmative = "yes";    // A 'yes' response
static const string userNegative = "no";        // A 'no' response to a prompt
static const string emptyCell = "-";            // Check for empty/uninhabited cells in Life files
static const int fileRowDimension = 0;          // Row in file that denotes the # of rows in grid
static const int fileColDimension = 1;          // Row in file that denotes the # of columns in grid
static const string stopWord = "quit";          // Keyword for user to end simulation in manual mode
static const int kNumCells = 800;               // Number of cells populated in a random Life grid
static const int kBoardDimension = 40;          // Number of rows/columns in a random Life grid
static const int kNumRows = kBoardDimension;
static const int kNumColumns = kBoardDimension;
static const int simSpeedMin = 1;
static const int simSpeedManual = 4;
static const int msToPauseMin = 1;
static const int msToPauseMid = 100;
static const int msToPauseMax = 500;

// Prototypes
static void welcome();
static void runGame(LifeDisplay& display);
static Grid<int> initializeGrid(ifstream& file);
static void readInGrid(ifstream& file, Grid<int>& currentGrid);
static void randomizeGrid(Grid<int>& currentGrid);
static void drawGrid(const Grid<int>& currentGrid, LifeDisplay& display);
static void runSimulation(Grid<int>& currentGrid, LifeDisplay& display);
static int setSimulationSpeed(int& pauseLength);
static void advanceGrid(Grid<int>& currentGrid, LifeDisplay& display, const Grid<int>& scratchGrid);
static Grid<int> calculateNextGeneration(const Grid<int>& currentGrid);
static int countNeighbors(const Grid<int>& currentGrid, int row, int col);
static bool isStable(const Grid<int>& currentGrid, const Grid<int>& scratchGrid);
static void continueGame(LifeDisplay& display);

static string newGetLine(string prompt);
static int newGetInteger(string prompt);

int main() {
    welcome();
    LifeDisplay display;
    display.setTitle("Game of Life");
    runGame(display);
    display.~LifeDisplay();
    return 0;
}

/* Print out greeting for beginning of program. */
static void welcome() {
    cout << "Welcome to the game of Life, a simulation of the lifecycle of a bacteria colony." << endl;
    cout << "Cells live and die by the following rules:" << endl << endl;
    cout << "\tA cell with 1 or fewer neighbors dies of loneliness" << endl;
    cout << "\tLocations with 2 neighbors remain stable" << endl;
    cout << "\tLocations with 3 neighbors will spontaneously create life" << endl;
    cout << "\tLocations with 4 or more neighbors die of overcrowding" << endl << endl;
    cout << "In the animation, new cells are dark and fade to gray as they age." << endl << endl;
    getLine("Hit [enter] to continue....   ");
}

/**
 * Procedure: runGame
 * This serves as a mini main program that houses the core functions
 * of loading/drawing the grid and playing the game. The rationale behind this
 * is we need a way of restarting the game if the user so chooses after running a simulation.
 * In an effort to not get in a parameter reference quandry or end up not
 * not calling a vital function, this serves as an option to restart.
 * Parameters: LifeDisplay display object by reference initialized in the main()
 */
static void runGame(LifeDisplay& display) {
    ifstream file;                                                  // File stream for a read-in file, if applicable
    Grid<int> currentGrid = initializeGrid(file);                   // Baseline grid used for initial construct passed by reference throughout
    display.setDimensions(currentGrid.numRows(), currentGrid.numCols());
    drawGrid(currentGrid, display);
    runSimulation(currentGrid, display);
    continueGame(display);
}

/**
 * Function: initializeGrid
 * This calls the subfunctions that will either read in a grid from a file (function: readInGrid)
 * or randomly create one (function: randomizeGrid), and ultimately returns it to the runGame procedure
 * so to be used throughout the rest of the program. It prompts the user
 * for their preference of reading or randomly creating, checks for invalid inputs
 * on both the randomization (function: newGetLine) and file path prompts (function: prompUserFile),
 * and then calls the associated function.
 * Parameters: filestream declared in runGame by reference
 */
static Grid<int> initializeGrid(ifstream& file) {
    Grid<int> currentGrid;
    string line;                                                    // String used to read in user's file input
    cout << "\nYou can start your colony with random cells or read from a prepared file." << endl;
    if(newGetLine("Do you have a starting file in mind? (yes/no) ") == userAffirmative) {
        cout << "Opened file named " << promptUserForFile(file, "Please enter filename: ", "Unable to open file. Please try again.") << "." << endl;
        readInGrid(file, currentGrid);
    } else {
        cout << "Okay, I will seed your colony randomly" << endl;
        randomizeGrid(currentGrid);
    }
    return currentGrid;
}

/**
 * Procedure: readInGrid
 * This references the file stream read in from initializeGrid and the baseline grid
 * used for the game, which initializeGrid returns to runGame.
 * readInGrid reads the file referenced into a vector so that it can remove
 * the commented lines atop files that should be ignored.
 * It then resizes the referenced grid based on the dimensions specified by the file.
 * Finally it reads all characters on the lines after those specifying the grid dimensions
 * into the referenced grid with the appropriate integer value if a cell is living.
 * I tried for a more elegant solution using while loops for character and file streams, but
 * didn't finish in the end.
 * Parameters: file stream for user-identified grid file and the base grid used in the game
 */
static void readInGrid(ifstream& file, Grid<int>& currentGrid) {
    Vector<string> linesInFile;                                     // Vector used to read in lines of file and remove extraneous
    readEntireFile(file, linesInFile);
    file.close();
    for(int i = linesInFile.size()-1; i >= 0; i--) {
        if(linesInFile[i].find("#") != string::npos) linesInFile.remove(i);
    }

    currentGrid.resize(stringToInteger(linesInFile[fileRowDimension]), stringToInteger(linesInFile[fileColDimension]));

    for(int numLines = fileColDimension+1; numLines < linesInFile.size(); numLines++) {
        for(int numChars = 0; numChars < linesInFile[numLines].length(); numChars++) {
            if(linesInFile[numLines].substr(numChars,1) == emptyCell) {
                currentGrid[numLines - (fileColDimension+1)][numChars] = 0;
            } else {
                currentGrid[numLines - (fileColDimension+1)][numChars] = 1;
            }
        }
    }
}

/**
 * Procedure: randomizeGrid
 * This procedure plots cells randomly on a life board given the
 * randomized constants specified. It calls on the Stanford random library
 * to return a random integer that is used to place a cell on as long as
 * there isn't one there.
 * Parameters: baseline grid by reference
 */
static void randomizeGrid(Grid<int>& currentGrid) {
    int numCellsPlaced = 0;                                         // Cell counter used to keep track of already placed
    currentGrid.resize(kNumRows, kNumColumns);
    while (numCellsPlaced < kNumCells) {
        int row = randomInteger(0, currentGrid.numRows() - 1);
        int col = randomInteger(0, currentGrid.numCols() - 1);
        if (!currentGrid[row][col]) {
            currentGrid[row][col] = 1;
            numCellsPlaced++;
        }
    }
}

/**
 * Procedure: drawGrid
 * Flexible procedure that reads by reference a grid as a constant and
 * its corresponding display object so to plot the grid in the
 * associated graphics window. It does this through two simple for loops
 * that go through each cell on the grid and indicate the age of the cell there.
 * Parameters: any grid by constant reference, referenced display object
 */
static void drawGrid(const Grid<int>& currentGrid, LifeDisplay& display) {
    for(int row = 0; row < currentGrid.numRows(); row++) {
        for(int col = 0; col < currentGrid.numCols(); col++) {
            display.drawCellAt(row, col, currentGrid[row][col]);
        }
    }
}

/**
 * Procedure: runSimulation
 * This procedure carries out most of the animation either directly through
 * mouse events or by calling additional functionals.  To determine the animation
 * speed for the simulation, it calls setSimulationSpeed to return the user's choice
 * and to pass the corresponding animation pause length. Once received, it
 * enters a while loop to animate the simulation through advanceGrid, provided that
 * the current grid is not stable (returned via function isStable) or the user has
 * not clicked the mouse in a non-manual mode. It notably employs the use of two
 * grids to compare the current state of the grid to the envisioned in the next
 * generation via calculateNextGeneration, which is a crucial step prior to advanceGrid.
 * Parameters: current grid by only reference since it can be changed, referenced display object
 */
static void runSimulation(Grid<int>& currentGrid, LifeDisplay& display) {
    int pauseLength;                                                    // Pause length, an integer for milliseconds, based on Cynthia's Piazza post (gwindow.h says double)
    int simulationSpeed = setSimulationSpeed(pauseLength);              // User-inputted simulation choice [1,4]
    while (true) {
        Grid<int> scratchGrid = calculateNextGeneration(currentGrid);
        if(isStable(currentGrid, scratchGrid)) break;
        advanceGrid(currentGrid, display, scratchGrid);

        if(simulationSpeed != simSpeedManual) {
            GMouseEvent me = getNextEvent(MOUSE_EVENT);
            if (me.getEventType() == MOUSE_CLICKED) {
                break;
            } else if (me.getEventType() == NULL_EVENT) {
                pause(pauseLength);
            }
        } else {
            if(getLine("Hit [enter] to continue (or \"quit\" to end the simulation): ") == stopWord) break;
        }
    }
}

/**
 * Function: setSimulationSpeed
 * A function that prompts a user for the desired simulation speed, which are controlled
 * by constant integers representings milliseconds via the pause length between animations.
 * The function takes pauseLength as a reference since it already is returning the user input.
 * It also prints out the specific direction to end a simulation by mouse for the non-manual case,
 * rather than having it repeat in runSimulation's while loop.
 * Parameters: pauseLength in milliseconds as an integer based on Cynthia's Piazza post
 */
static int setSimulationSpeed(int& pauseLength) {
    int simulationSpeed;                                                // Integer [1,4] based on user's input
    cout << "\nYou choose how fast to run the simulation." << endl;
    cout << "\t1 = As fast as this chip can go!" <<endl;
    cout << "\t2 = Not too fast; this is a school zone." << endl;
    cout << "\t3 = Nice and slow I can watch everything that happens." << endl;
    cout << "\t4 = Wait for user to hit enter between generations." << endl;
    simulationSpeed = newGetInteger("Your choice: ");
    switch(simulationSpeed) {
        case 1:
            pauseLength = msToPauseMin;
            cout << "\nClick and hold the mouse button on the graphics window to end the simulation." << endl;
            break;
        case 2:
            pauseLength = msToPauseMid;
            cout << "\nClick and hold the mouse button on the graphics window to end the simulation." << endl;
            break;
        case 3:
            pauseLength = msToPauseMax;
            cout << "\nClick and hold the mouse button on the graphics window to end the simulation." << endl;
            break;
    }
    return simulationSpeed;
}

/**
 * Procedure: advanceGrid
 * A simple procedure that could have been condensed into the runSimulation function.
 * It takes the current and envisioned grid generations by reference, the latter by constant,
 * and copies the latter into the former.  Once completed, it calls on the drawGrid procedure
 * to update the graphics to the new generation simultaneously.
 * Parameters: current and scratch grids, the latter by constant since it is only being read to the former,
 * and the display object
 */
static void advanceGrid(Grid<int>& currentGrid, LifeDisplay& display, const Grid<int>& scratchGrid) {
    currentGrid = scratchGrid;
    drawGrid(currentGrid, display);
}

/**
 * Function: calculateNextGeneration
 * Function that returns what the next generation's grid should be to runSimulation.
 * It does this by looping through each cell on the grid, calling a helper function
 * that calculates how many neighbors each cell it's looping through has, and
 * makes a determination as to that cell's age in the next generation per the rules.
 * Important that the increment operator on age comes before the the value as it needs
 * to be incremented first prior to assignment to the grid.
 * Parameters: current generation's grid by constant reference, since only the future's is being determined
 */
static Grid<int> calculateNextGeneration(const Grid<int>& currentGrid) {
    Grid<int> scratchGrid(currentGrid.numRows(), currentGrid.numCols());
    for(int row = 0; row < currentGrid.numRows(); row++) {
        for(int col = 0; col < currentGrid.numCols(); col++) {
            int age = currentGrid[row][col];
            int neighbors = countNeighbors(currentGrid, row, col);
            switch(neighbors) {
                case 2:
                    if(age > 0) {
                        scratchGrid[row][col] = ++age;
                    } else {
                        scratchGrid[row][col] = 0;
                    }
                    break;
                case 3:
                    scratchGrid[row][col] = ++age;
                    break;
                default:
                    scratchGrid[row][col] = 0;
                    break;
            }
        }
    }
    return scratchGrid;
}

/**
 * Function: countNeighbors
 * Function that returns the number of neighbors a grid has given
 * a grid and a specific cell reference on that grid.  Simple implementation
 * that loops through all the proximal cells to the one referenced, ensuring that those
 * proximal cells actually exist using the Grid class' inBounds function, and also
 * excluding the cell referenced from counting itself.
 * Parameters: the desired grid by constant reference since nothing is changing, and the specific
 * address desired for neighbor counting that grid
 */
static int countNeighbors(const Grid<int>& currentGrid, int row, int col) {
    int neighbors = 0;                                              // Counter for number of neighbors a cell has
    for(int rowOffset = -1; rowOffset <= 1; rowOffset++) {
        for(int colOffset = -1; colOffset <= 1; colOffset++) {
            if(currentGrid.inBounds(row+rowOffset, col+colOffset) && !(rowOffset == 0 && colOffset == 0)) {
                int age = currentGrid[row+rowOffset][col+colOffset];
                if(age > 0) neighbors++;
            }
        }
    }
    return neighbors;
}

/**
 * Predicate Function: isStable
 * Predicate function that returns true if the current generation is stable. This is done
 * by checking whether the cells are in the same locations, by proxy by taking the difference in the
 * cumulative ages and setting it equal to the number of cells, and by ensuring all cells
 * are over the maximum cell age.
 * Parameters: both generation grids by constant reference since nothing is changing
 */
static bool isStable(const Grid<int>& currentGrid, const Grid<int>& scratchGrid) {
    int currentGridAge = 0;
    int scratchGridAge = 0;
    int currentGridCells = 0;
    int scratchGridCells = 0;
    for(int row = 0; row < currentGrid.numRows(); row++) {
        for(int col = 0; col < currentGrid.numCols(); col++) {
            currentGridAge += currentGrid[row][col];
            scratchGridAge += scratchGrid[row][col];
            if(currentGrid[row][col] > 0) currentGridCells++;
            if(scratchGrid[row][col] > 0) scratchGridCells++;
        }
    }
    if((scratchGridAge >= scratchGridCells * kMaxAge) && (scratchGridAge - currentGridAge) == currentGridCells) {
        cout << "Stable Configuration" << endl;
        return true;
    } else {
        return false;
    }
}

/**
 * Procedure: continueGame
 * Simple function called prompting a user to continue with a new simulation, or
 * indirectly to exit by returning to runGame where the program exists.
 * Parameters: display object by reference since it's initialized in main and needed for runGame
 */
static void continueGame(LifeDisplay& display) {
    if(newGetLine("\nWould you like to run another simulation? (yes/no) ") == userAffirmative) {
        runGame(display);
    }
}

/**
 * Function: newGetLine
 * Alternative version of getLine that returns a custom reprompt
 * until the appropriate token is entered.
 */
static string newGetLine(string prompt) {
   string line;
   cout << prompt;
   while (true) {
      getline(cin, line);
      if (line == userAffirmative || line == userNegative) break;
      cout << "Please answer yes or no." << endl;
   }
   return line;
}

/**
 * Function: newGetInteger
 * Alternative version of getInteger that returns a custom reprompt
 * until the appropriate integer from 1-4 is entered.
 */
static int newGetInteger(string prompt) {
    int value;
    string line;
    cout << prompt;
    while (true) {
        getline(cin, line);
        istringstream stream(line);
        stream >> value >> ws;
        if (!stream.fail() && stream.eof() && value >= simSpeedMin && value <= simSpeedManual) break;
        cout << "\nPlease enter your choice between 1 and 4: ";
    }
    return value;
}
