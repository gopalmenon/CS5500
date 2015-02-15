#include <tbb\blocked_range.h>
#include <tbb\parallel_for.h>
#include <tbb\parallel_reduce.h>

#include "GameSlot.hpp"

#include <algorithm> 
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

class ConnectFourGame {

private:

	//Constants
	const static int DEFAULT_NUMBER_OF_ROWS = 6;
	const static int DEFAULT_NUMBER_OF_COLUMNS = 7;
	const static bool DEFAULT_FIRST_PLAYER_IS_USER = true;
	const static bool DEFAULT_DIFFICULTY_LEVEL = 0;

	int numberOfRows, numberOfColumns, gameDifficultyLevel;
	bool firstPlayerIsUser;
	std::vector<GameSlot> gameBoard;

	//Check if this is a valid play given the game board dimensions and coins already played
	bool isValidPlay(int dropInColumn) {

		//First check if the column number is valid as per the dimensions of the game board
		if (isValidColumn(dropInColumn)) {

			//Next check if there is at least one empty slot in the column. i.e. check if the top slot is empty
			if (gameBoard.at(dropInColumn).isEmpty()) {
				return true;
			}
			else {
				return false;
			}

		}
		else {
			return false;
		}

	}

	//Check if the column is valid according to the board dimensions
	bool isValidColumn(int columnNumber) {

		if (columnNumber >= 0 && columnNumber < this->numberOfColumns) {
			return true;
		}
		else {
			return false;
		}
	}

	//Called by the constructors. Fill the game board with empty slots
	void fillGameBoardWithEmptySlots(int numberOfRows, int numberOfColumns) {

		int numberOfSlotsOnGameBoard = numberOfRows * numberOfColumns;
		//TODO check if this is worth doing in parallel
		for (int slotCounter = 0; slotCounter < numberOfSlotsOnGameBoard; ++slotCounter) {
			gameBoard.emplace_back(GameSlot());
		}

	}

	//Return the index corresponding to the top available position
	int getAvailableSlot(int columnNumber) {

		//Make sure that this is a valid play
		assert(isValidPlay(columnNumber));

		//Find the top and bottom rows in the columns
		int bottomRowInColumn = columnNumber + this->numberOfColumns * (this->numberOfRows - 1);
		int topRowInColumn = columnNumber;

		//Start with bottom row and go one cell above at a time till an empty one is found
		for (int slotCounter = bottomRowInColumn; slotCounter >= topRowInColumn; slotCounter -= this->numberOfColumns) {
			if (this->gameBoard.at(slotCounter).isEmpty()) {
				return slotCounter;
			}
		}

		//Exit the program as if it reaches here as this should never happen
		assert(false);

		return 0;
	}

	//Drop a coin into one of the columns
	void dropCoin(int dropInColumn, bool isUserCoin) {

		//First check if the play is a valid one
		if (this->isValidPlay(dropInColumn)) {

			//Place a user coin in the top available position
			this->gameBoard.at(getAvailableSlot(dropInColumn)).putCoin(isUserCoin);

			if (wasWinningPlay(dropInColumn, true)) {
				endTheGame(true);
			}
			else {
				//Make a move to best counter the user move
				counterUserMove();
			}
		}
	}


	//Check if the last play was a winning play
	bool wasWinningPlay(int columnPlayed, bool playedByUser) {

		//TODO check for horizontal, vertical and two diagonal winning play
		return false;

	}
	
	//Show message saying who won and disable game controls
	void endTheGame(bool userWon) {

		//TODO display message saying the user won/lost and disable game playing controls
	}

	//Consider all possible moves and play the one with the best hueristic score that maximizes the chance of winning 
	void counterUserMove() {

		std::vector<int> moveScores;

		//Find best move by considering all columns in parallel using the Map pattern
		tbb::parallel_for(
			tbb::blocked_range<int>(0, this->numberOfColumns),
			[&](tbb::blocked_range<int> range) {
				for (size_t coulumnCounter = range.begin(); coulumnCounter != range.end(); ++coulumnCounter) {
					moveScores.emplace_back(getMoveHueristicScore(coulumnCounter, true, this->gameDifficultyLevel));
				}
			}
		);

		//Do a parallel reduction to find the move with the highest score
		tbb::parallel_reduce(
			tbb::blocked_range<int>(0, this->numberOfColumns),
			0,
			[=](const tbb::blocked_range<int>& range, int moveColumn)->int {
				for (int columnCounter = range.begin(); columnCounter != range.end(); ++columnCounter) {
					if (moveScores.at(columnCounter) > moveColumn) {
						moveColumn = moveScores.at(columnCounter);
					}
				}
				return moveColumn;
			},
			[](int moveColumn1, int moveColumn2)->int {
				if (moveColumn1 > moveColumn2) {
					return moveColumn1;
				}
				else {
					return moveColumn2;
				}
			}
		);
	}

	int getMoveHueristicScore(int columnPlayed, bool isRespondingToUserPlay, int recursionDepth) {

		//TODO implement hueristics to return score of the move
		return 1;

	}

public:

	//Default constructor will set game parameters using default values
	ConnectFourGame() {
		this->numberOfRows = DEFAULT_NUMBER_OF_ROWS;
		this->numberOfColumns = DEFAULT_NUMBER_OF_COLUMNS;
		this->firstPlayerIsUser = DEFAULT_FIRST_PLAYER_IS_USER;
		this->gameDifficultyLevel = DEFAULT_DIFFICULTY_LEVEL;
		fillGameBoardWithEmptySlots(this->numberOfRows, this->numberOfColumns);
		//TODO computer to go first depending on user setting
	}

	ConnectFourGame(int numberOfRows, int numberOfColumns) {
		this->numberOfRows = numberOfRows;
		this->numberOfColumns = numberOfColumns;
		fillGameBoardWithEmptySlots(this->numberOfRows, this->numberOfColumns);
		//TODO computer to go first depending on user setting
	}

	//First player will be determined by user selection
	void setWhoPlaysFirst(bool firstPlayerIsUser) {
		this->firstPlayerIsUser = firstPlayerIsUser;
	}

	//Difficulty level will be set by user
	void setgameDifficultyLevel(int gameDifficultyLevel) {
		this->gameDifficultyLevel = gameDifficultyLevel;
	}

	//User method to drop a coin into one of the columns
	void dropCoin(int dropInColumn) {
		
		dropCoin(dropInColumn, true);
	}

};