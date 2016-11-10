/*File Name: rbs.c
**Program Purpose: Program runs the red/blue computation with parameters specified by the user.
**Author: Karol Zdebel 
**Student #: 0892519*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <string.h>

#ifndef _WIN32 // if we're not on Windows
#define _POSIX_C_SOURCE 200112L // get barriers on Linux
#endif

#define UNINIT -1
#define WHITE 0
#define RED 1
#define BLUE 2
#define STEP 2
#define HALFSTEP 1
#define AUTO 0
#define OUT_FILE -1

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
	char boardString[boardSize*boardSize+boardSize+5];
	int curChar =0;

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

int **getThresholdArgs(int procNum){
	int **args = malloc(sizeof(int*)*procNum);
	int threadsUsed;
	int tileNum = (boardSize*boardSize)/(tileSize*tileSize);
	int threadsPerTile;
	int tilesExtra;
	int curTile=1;

	if (procNum > tileNum){
		threadsUsed = tileNum;		//Use less threads since more arent needed
	}
	else{
		threadsUsed = procNum;		//Use all thread
	}

	threadsPerTile = tileNum/threadsUsed;
	tilesExtra = tileNum-(threadsPerTile*threadsUsed);

	for (int i=0;i<procNum;i++){
		args[i] = malloc(sizeof(int)*2);
		args[i][0] = curTile;				//Tile this thread will begin on

		args[i][1] = threadsPerTile;		//Amount of tiles this thread will read

		if (tilesExtra > 0){
			args[i][1]++;					//Add any remaining tiles to this thread
			tilesExtra--;
		}

		curTile += args[i][1];				//Next tile starts on the tile following where previous thread finished
	}

	return args;
}

void *checkThreshold(void *arg){
	int startingTile = ((int*)arg)[0];	//Tile to begin computation on
	int tileNum = ((int*)arg)[1];		//Number of tiles to read
	int red, blue;						//Number of red, blue tiles
	double redDen, blueDen;				//Red and blue tile density
	double highestDen =0;
	Position startPos;					//Starting position of tile

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

	pthread_exit(EXIT_SUCCESS);

}

void *moveRed(void *arg){

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

	pthread_exit(EXIT_SUCCESS);
}

void *moveBlue(void *arg){

	int **copy = boardCopy();
	int steps;
	Position pos;
	Position curPos;

	pos.row = ((int*)arg)[0];
	pos.col = ((int*)arg)[1];
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

	pthread_exit(EXIT_SUCCESS);
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
	if (cmdArgs[6] == true){
		sprintf(args,"\ti:true");
		fprintf(stdout,"\ti:true");
		fputs(args,file);
	}

	sprintf(args,"\trequired steps:%d\thighest tile density:%d\ttime:%lf\n",steps,highestDen,compTime);
	fprintf(stdout,"\trequired steps:%d\thighest tile density:%d\ttime:%lf\n",steps,highestDen,compTime);
	fputs(args,file);
	exit(EXIT_SUCCESS);
}

int getInput(){
	char input[1000];
	int stepNum=0;

	fprintf(stdout,"<Enter>: i.e., empty input; perform one full step\n");
	fprintf(stdout,"#: perform (integer) # full steps\n");
	fprintf(stdout,"h: perform one half step\n");
	fprintf(stdout,"c: continue in automatic mode to normal termination and output file\n");
	fprintf(stdout,"x: output current board into file and exit\n");

	while (true){
		if (fgets(input, 1000, stdin) == NULL){
			fprintf(stdout,"Error\n");
			continue;
		}
		switch(input[0]){

			case '\n':
				return STEP;
				break;

			case 'h':
				return HALFSTEP;
				break;

			case 'c':
				return AUTO;
				break;

			case 'x':
				return OUT_FILE;
				break;
			default:
				stepNum = strtol(input,NULL,10);
				if (stepNum > 0){
					return STEP*stepNum;	
				}
				else{
					fprintf(stdout,"Error\n");
				}
		}
		fflush(stdin);
	}

	return STEP;
}

int main(int argc, char **argv){
	int procNum = UNINIT;	// pN: no. of processors (threads) to use
	int maxSteps = UNINIT;	// mN: max. no. of full steps (additional stopping condition)
	int randSeed = UNINIT;	// sN: optional random seed
	bool interMode = false;	// i: optional interactive mode switch
	int *threadSteps;		// How many steps each thread will executing
	int **threadArg;		// Arguments stored in array for each thread
	int **threshArg;
	int procThresh;
	int tileNum;
	int input;
	int completedSteps=0;
	int stepsTillInter = 0;
	double topDen=0;
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

			case 'i':
				interMode = true;
				break;
		}
	}

	int cmdArgs[7] = {procNum,boardSize,tileSize,(int)(colourDen*100),maxSteps,randSeed,interMode};

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
	threshArg = getThresholdArgs(procNum);

	//Create thread ID's
	threadId = malloc(sizeof(pthread_t)*procNum);

	tileNum = (boardSize*boardSize)/(tileSize*tileSize);
	tileDen = malloc(sizeof(int)*tileNum);
	
	if (procNum > tileNum){
		procThresh = tileNum;
	}
	else{
		procThresh = procNum;
	}

	//Allow each thread to execute its steps on the board
	for (int i=0;i<maxSteps;i++){

		//interactive mode
		if (interMode && stepsTillInter == 0){
			input = getInput();

			if (input == HALFSTEP && stepsTillInter == 0){
				stepsTillInter = HALFSTEP;
			}
			else if (input == AUTO){
				interMode = false;
			}
			else if (input == OUT_FILE){
				//write to file
			}
			else if (input == STEP){
				stepsTillInter = STEP;
			}
			else{
				stepsTillInter = input;
			}

		}

		topDen=0;
		
		//check threshold
		for (int k=0;k<procThresh;k++){
			pthread_create(&(threadId[k]),NULL,checkThreshold,threshArg[k]);
		}
		for (int k=0;k<procThresh;k++){
			pthread_join(threadId[k],NULL);
		}

		//check highest density
		for (int k=0;k<tileNum;k++){
			if (tileDen[k] > topDen){
				topDen = tileDen[k];
			}

			//check for termination
			if (topDen > (int)(100*colourDen)){
				finishProgram(0.0,cmdArgs,completedSteps,topDen);
			}
		}

		//move red
		for (int k=0;k<procNum;k++){
			pthread_create(&(threadId[k]),NULL,moveRed,threadArg[k]);
		}
		for (int k=0;k<procNum;k++){
			pthread_join(threadId[k],NULL);
		}

		if (interMode && stepsTillInter > 0){
			stepsTillInter--;
		}
		if (interMode && stepsTillInter == 0){
			input = getInput();

			if (input == HALFSTEP && stepsTillInter == 0){
				stepsTillInter = HALFSTEP;
			}
			else if (input == AUTO){
				interMode = false;
			}
			else if (input == OUT_FILE){
				//write to file
			}
			else if (input == STEP){
				stepsTillInter = STEP;
			}
			else{
				stepsTillInter = input;
			}
		}

		//move blue
		for (int k=0;k<procNum;k++){
			pthread_create(&(threadId[k]),NULL,moveBlue,threadArg[k]);
		}

		if (interMode && stepsTillInter > 0){
			stepsTillInter--;
		}

		for (int k=0;k<procNum;k++){
			pthread_join(threadId[k],NULL);
		}

		completedSteps++;
	}

	finishProgram(0.0,cmdArgs,completedSteps,topDen);

	free(threadSteps);
	free(startingPos);
	free(threadId);
	free(tileDen);
	freeBoard(&board);
	freeArg(&threshArg,procNum);
	freeArg(&threadArg,procNum);

	return 0;
}
