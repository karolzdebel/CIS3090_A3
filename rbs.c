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

typedef struct startPos{
	int col;
	int row;
}startPos;

static startPos getStartPos(int *threads,int curThread, int boardSize){
	startPos pos;
	int prevSteps = 0;

	if (curThread == 0){
		pos.col = 0;
		pos.row = 0;
		return pos;
	}
	pos.row = (threads[curThread]/boardSize)-1;
	pos.col = (prevSteps%boardSize)-1;
	
	return pos;
}

static void freeBoard(int size,int ***board){
	for (int i=0;i<size;i++){
		free((*board)[i]);
	}
	free(*board);
}

static int *getThreadSteps(int boardSize, int threads){
	int totalSteps = boardSize*boardSize;
	int extraSteps = totalSteps%threads;
	int *threadSteps = malloc(sizeof(int)*threads);


	for (int i=0;i<threads;i++){
		//Give each thread a proportional amount of threads
		threadSteps[i] += i*(totalSteps/threads);
		
		//Any remaining steps due to uneven division distribute among threads 
		if (extraSteps > 0){
			threadSteps[i+1]++;
			extraSteps--;
		}
	}

	return threadSteps;
}

static void printBoard(int boardSize, int **board){
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

static int **boardCopy(int size, int **board){
	int **boardCopy = malloc(sizeof(int*)*size);
	
	for (int i=0;i<size;i++){
		boardCopy[i] = malloc(sizeof(int)*size);
		
		for (int k=0;k<size;k++){
			boardCopy[i][k] = board[i][k];
		}
	}
	return boardCopy;
}

static void moveRed(int size, int row,int ***board){

	int **copy = boardCopy(size,*board);

	for (int i=0;i<size-1;i++){

		//Move any reds with a white to the right
		if (copy[row][i] == RED && copy[row][i+1] == WHITE){
			(*board)[row][i] = WHITE; 
			(*board)[row][i+1] = RED;
		}
		//Move last red if the first element is white(wrap around)
		if (i==size-2){
			if (copy[row][size-1] == RED && copy[row][0] == WHITE){
				(*board)[row][size-1] = WHITE;
				(*board)[row][0] = RED;
			}
		}
	}
}

static void moveBlue(int size, int col,int ***board){

	int **copy = boardCopy(size,*board);

	for (int i=0;i<size-1;i++){

		//Move any reds with a white to the right
		if (copy[i][col] == BLUE && copy[i+1][col] == WHITE){
			(*board)[i][col] = WHITE; 
			(*board)[i+1][col] = BLUE;
		}
		//Move last red if the first element is white(wrap around)
		if (i==size-2){
			if (copy[size-1][col] == BLUE && copy[0][col] == WHITE){
				(*board)[size-1][col] = WHITE;
				(*board)[0][col] = BLUE;
			}
		}
	}
}

// static bool checkThreshold(int size, int threshold, int **board){

// }

static int **createBoard(int size, int seed){
	int **board = malloc(sizeof(int*)*size);
	
	if (seed == -1){
		time_t t;
		srand((unsigned) time(&t));
	}
	else{
		srand(seed);		
	}

	for (int i=0;i<size;i++){
		board[i] = malloc(sizeof(int)*size);

		for (int k=0;k<size;k++){
			board[i][k] = rand()%3;
		}
	}

	return board;
}

int main(int argc, char **argv){
	int procNum = UNINIT;	// pN: no. of processors (threads) to use
	int boardSize = UNINIT;	// bN: width of board >1, so board size is N x N cells
	int tileSize = UNINIT;	// tN: width of one overlay tile, each N x N cells (b mod t=0)
	int colourDen = UNINIT;	// cN: max. colour density in integer percent, 1-100 (stopping condition)
	int maxSteps = UNINIT;	// mN: max. no. of full steps (additional stopping condition)
	int randSeed = UNINIT;	// sN: optional random seed
	bool interMode = false;	// i: optional interactive mode switch
	int **board;			// Board with white, red or blue positions
	int *threadSteps;		// How many steps each thread will executing
	startPos *startingPos;  // What row and column each thread will begin computation at

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

	printf("procNum:%d\tboardSize:%d\ttileSize:%d\tcolourDen:%d\tmaxSteps:%d\trandSeed:%d\tinterMode:%d\n"
		,procNum,boardSize,tileSize,colourDen,maxSteps,randSeed,interMode);

	board = createBoard(boardSize,randSeed);
	threadSteps = getThreadSteps(boardSize,procNum);


	for (int i=0;i<procNum;i++){
		printf("Thread %d: starting step:%d\tstarting col:%d\tstarting row:%d\n",i,threadSteps[i],);
	}

	freeBoard(boardSize,&board);

	return 0;
}
