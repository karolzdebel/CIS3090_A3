/*File Name: rbs.c
**Program Purpose: Program runs the red/blue computation with parameters specified by the user.
**Author: Karol Zdebel 
**Student #: 0892519*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#ifndef _WIN32 // if we're not on Windows
#define _POSIX_C_SOURCE 200112L // get barriers on Linux
#endif

#define UNINIT -1
#define WHITE 0
#define RED 1
#define BLUE 2

typedef struct Position{
	int col;
	int row;
}Position;

int **board;		// Board with white, red or blue positions
int colourDen;		// cN: max. colour density in integer percent, 1-100 (stopping condition)
int tileSize;		// tN: width of one overlay tile, each N x N cells (b mod t=0)
int boardSize;		// bN: width of board >1, so board size is N x N cells


Position getPosition(int *threads,int curThread, int boardSize){
	Position pos;

	if (curThread == 0){
		pos.col = 0;
		pos.row = 0;
		return pos;
	}
	pos.row = (threads[curThread]/boardSize);
	pos.col = (threads[curThread]%boardSize);
	
	return pos;
}

void freeBoard(int ***board){
	for (int i=0;i<boardSize;i++){
		free((*board)[i]);
	}
	free(*board);
}

void freeArg(int ***arg, int size){
	for (int i=0;i<size;i++){
		free((*arg)[i]);
	}
	free(*arg);
}

int *getThreadSteps(int boardSize, int threads){
	int totalSteps = boardSize*boardSize;
	int extraSteps = totalSteps%threads;
	int *threadSteps = malloc(sizeof(int)*threads);


	for (int i=0;i<threads;i++){
		//Give each thread a proportional amount of threads
		threadSteps[i] = i*(totalSteps/threads);
		
		//Any remaining steps due to uneven division distribute among threads 
		if (extraSteps > 0){
			threadSteps[i+1]++;
			extraSteps--;
		}
	}

	return threadSteps;
}

void printBoard(int boardSize, int **board){
	for (int i=0;i<boardSize;i++){
		for(int k=0;k<boardSize;k++){

			if (board[i][k] == WHITE){
				printf("~");	
			}
			else if (board[i][k] == RED){
				printf("R");
			}
			else if(board[i][k] == BLUE){
				printf("B");
			}

			if (k==boardSize-1){
				printf("\n");
			}
		}
	}
}

int **boardCopy(){
	int **boardCopy = malloc(sizeof(int*)*boardSize);

	for (int i=0;i<boardSize;i++){
		boardCopy[i] = malloc(sizeof(int)*boardSize);
		
		for (int k=0;k<boardSize;k++){
			boardCopy[i][k] = board[i][k];
		}
	}
	return boardCopy;
}

void moveRed(void *arg){

	int **copy = boardCopy();
	int steps;
	Position pos;
	Position curPos;

	pos.col = ((int*)arg)[0];
	pos.row = ((int*)arg)[1];
	steps = ((int*)arg)[2];

	curPos = pos;

	//Move reds in assigned positions
	for (int i=0;i<steps;i++){

		//Move last red if the first element is white(wrap around)
		if (curPos.col == (boardSize-1) ){
			if (copy[curPos.row][curPos.col] == RED && copy[curPos.row][0] == WHITE){
				board[curPos.row][curPos.col] = WHITE;
				board[curPos.row][0] = RED;
			}
		}

		//Move any reds with a white to the right
		else if (copy[curPos.row][curPos.col] == RED && copy[curPos.row][curPos.col+1] == WHITE){
			board[curPos.row][curPos.col] = WHITE; 
			board[curPos.row][curPos.col+1] = RED;
		}

		//Check element to right if there are any left, otherwise go to next rows
		if (curPos.col == (boardSize-1) ){
			curPos.row++;
			curPos.col=0;
		}
		else{
			curPos.col++;
		}
	}
	freeBoard(&copy);
}

void moveBlue(void *arg){

	int **copy = boardCopy();
	int steps;
	Position pos;
	Position curPos;

	pos.col = ((int*)arg)[0];
	pos.row = ((int*)arg)[1];
	steps = ((int*)arg)[2];

	curPos = pos;

	//Move blues in assigned positions
	for (int i=0;i<steps;i++){

		//Move last blue if the first element is white(wrap around)
		if (curPos.row == (boardSize-1) ){
			if (copy[curPos.row][curPos.col] == BLUE && copy[0][curPos.col] == WHITE){
				board[curPos.row][curPos.col] = WHITE;
				board[0][curPos.col] = BLUE;
			}
		}
		//Move any blues with a white underneath
		else if (copy[curPos.row][curPos.col] == BLUE && copy[curPos.row+1][curPos.col] == WHITE){
			board[curPos.row][curPos.col] = WHITE; 
			board[curPos.row+1][curPos.col] = BLUE;
		}

		//Check element to right if there are any left, otherwise go to next rows
		if (curPos.row == (boardSize-1) ){
			curPos.col++;
			curPos.row=0;
		}
		else{
			curPos.row++;
		}
	}

	freeBoard(&copy);
}

int **createBoard(int seed){
	int **newBoard = malloc(sizeof(int*)*boardSize);
	
	if (seed == -1){
		time_t t;
		srand((unsigned) time(&t));
	}
	else{
		srand(seed);		
	}

	for (int i=0;i<boardSize;i++){
		newBoard[i] = malloc(sizeof(int)*boardSize);

		for (int k=0;k<boardSize;k++){
			newBoard[i][k] = rand()%3;
		}
	}

	return newBoard;
}

int **getThreadArgs(int threadNum, int *threadSteps, Position *startingPos){
	int **arg = malloc(sizeof(int*)*threadNum);
	int steps;

	for (int i=0;i<threadNum;i++){

		if (i==threadNum-1){
			steps = (boardSize*boardSize)-threadSteps[i];
		}
		else{
			steps = threadSteps[i+1]-threadSteps[i];
		}

		arg[i] = malloc(sizeof(int)*3);
		arg[i][0] = startingPos[i].col;
		arg[i][1] = startingPos[i].row;
		arg[i][2] = steps;
	}

	return arg;
}

int main(int argc, char **argv){
	int procNum = UNINIT;	// pN: no. of processors (threads) to use
	int maxSteps = UNINIT;	// mN: max. no. of full steps (additional stopping condition)
	int randSeed = UNINIT;	// sN: optional random seed
	bool interMode = false;	// i: optional interactive mode switch
	int *threadSteps;		// How many steps each thread will executing
	int **threadArg;		// Arguments stored in array for each thread
	Position *startingPos;  // What row and column each thread will begin computation at

	colourDen = UNINIT;
	tileSize = UNINIT;
	boardSize = UNINIT;

	//Get all program parameters from arguments
	for (int i=1;i<argc;i++){
		switch(argv[i][0]){

			case 'p':
				argv[i][0] = ' ';

				if ( (procNum = strtol(argv[i],NULL,10)) < 1){
					fprintf(stderr,"ERROR: Number of processors cannot be less than 1. Exiting.\n");
					return(EXIT_FAILURE);
				}
				break;

			case 'b':
				argv[i][0] = ' ';

				if ( (boardSize = strtol(argv[i],NULL,10)) < 2){
					fprintf(stderr,"ERROR: Board size cannot be less than 2. Exiting.\n");
					return(EXIT_FAILURE);	
				}
				break;

			case 't':
				argv[i][0] = ' ';

				tileSize = strtol(argv[i],NULL,10);
				if (tileSize < 1){
					fprintf(stderr,"ERROR: Tile size must be greater than 1. Exiting.\n");
				}
				break;

			case 'c':
				argv[i][0] = ' ';

				colourDen = strtol(argv[i],NULL,10);
				if (colourDen > 100 || colourDen < 0){
					fprintf(stderr,"ERROR: Colour density outside of 0-100 range. Exiting.\n");
					return(EXIT_FAILURE);
				}
				break;

			case 'm':
				argv[i][0] = ' ';
				maxSteps = strtol(argv[i],NULL,10);
				break;

			case 's':
				argv[i][0] = ' ';
				randSeed = strtol(argv[i],NULL,10);
				break;

			case 'i':
				interMode = true;
				break;
		}
	}

	if (procNum == UNINIT || boardSize == UNINIT 
			|| tileSize == UNINIT || colourDen == UNINIT 
			|| maxSteps == UNINIT){

		fprintf(stderr,"ERROR: All required parameters have not been specified. Exiting.\n");
		return(EXIT_FAILURE);
	}

	if ( (boardSize % tileSize ) != 0){
		fprintf(stderr,"ERROR: Tile size is not a factor of board size. Exiting.\n");
		return(EXIT_FAILURE);
	}

	board = createBoard(randSeed);						//Generate the board
	threadSteps = getThreadSteps(boardSize,procNum);	//Get the number of steps each thread will perform
	
	//Get starting position for each thread
	startingPos = malloc(sizeof(Position)*procNum);		
	for (int i=0;i<procNum;i++){
		startingPos[i] = getPosition(threadSteps,i,boardSize);
	}

	//Get arguments for each thread
	threadArg = getThreadArgs(procNum,threadSteps,startingPos);

	//Allow each thread to execute its steps on the board
	int steps;
	for (int i=0;i<maxSteps;i++){
		for (int k=0;k<procNum;k++){
			moveRed( (void*)threadArg[k] );
		
		}

		for (int k=0;k<procNum;k++){
			moveBlue( (void*)threadArg[k] );
		
		}
	}

	free(threadSteps);
	free(startingPos);
	freeBoard(&board);
	freeArg(&threadArg,procNum);

	return 0;
}
