/*File Name: rbs.c
**Program Purpose: Program runs the red/blue computation with parameters specified by the user.
**Author: Karol Zdebel 
**Student #: 0892519*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define UNINIT -1
#define WHITE 0
#define RED 1
#define BLUE 2
#define STEP 2
#define HALFSTEP 1
#define AUTO 0
#define OUT_FILE -1
#define true 1
#define false 0

typedef struct Position{
	int col;
	int row;
}Position;

int **board;			// Board with white, red or blue positions
double colourDen;			// cN: max. colour density in integer percent, 1-100 (stopping condition)
int tileSize;			// tN: width of one overlay tile, each N x N cells (b mod t=0)
int boardSize;			// bN: width of board >1, so board size is N x N cells
pthread_t *threadId;	// Ids of all threads
int *tileDen;			//Highest density in each tile

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

void printBoard(int boardSize, int **board){
	for (int i=0;i<boardSize;i++){
		for(int k=0;k<boardSize;k++){

			if (board[i][k] == WHITE){
				fprintf(stdout,"=");	
			}
			else if (board[i][k] == RED){
				fprintf(stdout,">");
			}
			else if(board[i][k] == BLUE){
				fprintf(stdout,"V");
			}

			if (k==boardSize-1){
				fprintf(stdout,"\n");
			}
		}
	}
}

void writeBoard(int boardSize, int **board, FILE *file){
	char boardString[(boardSize*boardSize)+boardSize+5];
	int curChar =0;

	//initialize string
	for (int i=0;i<((boardSize*boardSize)+boardSize+5);i++){
		boardString[i] = ' ';
	}
	boardString[((boardSize*boardSize)+boardSize+4)] = '\0';

	for (int i=0;i<boardSize;i++){
		for(int k=0;k<boardSize;k++){

			if (board[i][k] == WHITE){
				boardString[curChar] = '=';	
			}
			else if (board[i][k] == RED){
				boardString[curChar] = '>';
			}
			else if(board[i][k] == BLUE){
				boardString[curChar] = 'V';
			}
			curChar++;

			if (k==boardSize-1){
				boardString[curChar] = '\n';
				curChar++;
			}
		}
	}
	fputs(boardString,file);

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

void progTerminate(){
	printf("program terminated;");
	exit(0);
}

bool checkThreshold(int *den){
	int startingTile = 1;											//Tile to begin computation on
	int tileNum = (boardSize*boardSize)/(tileSize*tileSize);		//Number of tiles to read
	int red, blue;													//Number of red, blue tiles
	int topDen = 0;
	int *tileDen = malloc(sizeof(int)*tileNum);
	double redDen, blueDen;											//Red and blue tile density
	double highestDen = 0;
	Position startPos;												//Starting position of tile

	//Go through all assigned tiles
	for (int j=0;j<tileNum;j++){

		red = blue = 0;	//Reset counters

		//Find position of tile
		if (startingTile == 1){
			startPos.col = 0;
		}
		else{
			startPos.col = ((startingTile-1)*tileSize) % boardSize;
		}
		if (startingTile == 1){
			startPos.row = 0;
		}
		else{
			startPos.row = tileSize*((tileSize*(startingTile-1))/boardSize);
		}

		//Go through all elements in tile and count red and blue
		for (int i=startPos.row;i<startPos.row+tileSize;i++){
			for (int k=startPos.col;k<startPos.col+tileSize;k++){
				if (board[i][k] == RED){
					red++;
				}
				else if (board[i][k] == BLUE){
					blue++;
				}
			}
		}

		//Check red ratio
		redDen = (double)red/(tileSize*tileSize);
		blueDen = (double)blue/(tileSize*tileSize);

		if (redDen > highestDen){
			highestDen = redDen;
		}
		if(blueDen > highestDen){
			highestDen = blueDen;
		}

		tileDen[startingTile-1] = (int)(100*highestDen);

		//Go to next tilee;
		startingTile++;
	}

	//check highest density
	for (int k=0;k<tileNum;k++){
		if (tileDen[k] > topDen){
			topDen = tileDen[k];
		}

		//check for termination
		if (topDen > (int)(100*colourDen)){
			*den = topDen;	//return by reference the highest density
			free(tileDen);
			return false;
		}
	}
	free(tileDen);
	return true;
}

void *moveRed(){

	int **copy = boardCopy();
	Position pos;
	Position curPos;

	pos.col = 0;
	pos.row = 0;

	curPos = pos;

	//Move reds in assigned positions
	for (int i=0;i<boardSize*boardSize;i++){

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

	return NULL;
}

void *moveBlue(){

	int **copy = boardCopy();
	Position pos;
	Position curPos;

	pos.row = 0;
	pos.col = 0;

	curPos = pos;

	//Move blues in assigned positions
	for (int i=0;i<boardSize*boardSize;i++){

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

	return NULL;
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

void finishProgram(double compTime, int cmdArgs[], int steps, int highestDen){
	FILE *file = fopen("redblue.txt","w+");
	char args[1000];
	//Write board to file

	writeBoard(boardSize,board,file);

	sprintf(args,"pN:%d\tbN:%d\ttN:%d\tcN:%d\tmN:%d",cmdArgs[0],cmdArgs[1],cmdArgs[2],cmdArgs[3],cmdArgs[4]);
	fprintf(stdout,"pN:%d\tbN:%d\ttN:%d\tcN:%d\tmN:%d",cmdArgs[0],cmdArgs[1],cmdArgs[2],cmdArgs[3],cmdArgs[4]);
	fputs(args,file);

	if (cmdArgs[5] != UNINIT){
		sprintf(args,"\tsN:%d",cmdArgs[5]);
		fprintf(stdout,"\tsN:%d",cmdArgs[5]);
		fputs(args,file);
	}

	sprintf(args,"\trequired steps:%d\thighest tile density:%d\ttime:%lf\n",steps,highestDen,compTime);
	fprintf(stdout,"\trequired steps:%d\thighest tile density:%d\ttime:%lf\n",steps,highestDen,compTime);
	fputs(args,file);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv){
	int procNum = UNINIT;	// pN: no. of processors (threads) to use
	int maxSteps = UNINIT;	// mN: max. no. of full steps (additional stopping condition)
	int randSeed = UNINIT;	// sN: optional random seed
	int completedSteps=0;
	int topDen=0;

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
				colourDen /= 100;
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
		}
	}

	int cmdArgs[6] = {procNum,boardSize,tileSize,(int)(colourDen*100),maxSteps,randSeed};

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

	clock_t start = clock();
	//Allow each thread to execute its steps on the board
	for (int i=0;i<maxSteps;i++){

		if (checkThreshold(&topDen)){
			finishProgram((clock()-start)/CLOCKS_PER_SEC,cmdArgs,completedSteps,topDen);
		}

		moveRed();
		moveBlue();

		completedSteps++;
	}

	finishProgram((clock()-start)/CLOCKS_PER_SEC,cmdArgs,completedSteps,topDen);

	freeBoard(&board);

	return 0;
}
