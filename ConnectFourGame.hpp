#ifndef CONNECTFOURGAME
#define CONNECTFOURGAME

#include <tbb\blocked_range.h>
#include <tbb\parallel_for.h>

#include "GameBoard.hpp"
#include "GameSlot.hpp"

#include <algorithm> 
#include <climits>
#include <iostream>
#include <utility>
#include <vector>

namespace controller {

	class ConnectFourGame {

	private:

		//Constants
		const static bool DEFAULT_FIRST_PLAYER_IS_USER = true;
		const static bool DEFAULT_MODE_IS_PARALLEL = true;
		const static int DEFAULT_DIFFICULTY_LEVEL = 2;
		const static int HEURISTIC_SCORE_FOR_ONE_IN_ROW = 1;
		const static int HEURISTIC_SCORE_FOR_TWO_IN_ROW = 3;
		const static int HEURISTIC_SCORE_FOR_THREE_IN_ROW = 9;
		const static int HEURISTIC_SCORE_FOR_FOUR_IN_ROW = INT_MAX;
		const static int COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW = 3;
		const static int HEURISTIC_SCORE_DIRECTIONS = 4;

		int gameDifficultyLevel;
		bool firstPlayerIsUser, gameModeIsParallel, gameIsOver, userWonTheGame;
		model::GameBoard gameBoard;

		//Get heuristic scores for the four possible directions
		void getHueristicScores(int& horizontalHueristicScore, int& verticalHueristicScore, int& positiveSlopeHueristicScore, int& negativeSlopeHueristicScore, int columnPlayed, const model::GameBoard gameBoard, bool isUserCoin) {

			if (this->gameModeIsParallel) {

				tbb::parallel_for(
					tbb::blocked_range<int>(0, gameBoard.getNumberOfColumns()),
					[=, &horizontalHueristicScore, &verticalHueristicScore, &positiveSlopeHueristicScore, &negativeSlopeHueristicScore](tbb::blocked_range<int> range) {

					for (int counter = 0; counter < HEURISTIC_SCORE_DIRECTIONS; ++counter) {

						switch (counter) {

						case 0:
							horizontalHueristicScore = getHorizontalHueristicScore(columnPlayed, gameBoard, isUserCoin);
							break;

						case 1:
							verticalHueristicScore = getVerticalHueristicScore(columnPlayed, gameBoard, isUserCoin);
							break;

						case 2:
							positiveSlopeHueristicScore = getPositiveSlopeHueristicScore(columnPlayed, gameBoard, isUserCoin);
							break;

						case 3:
							negativeSlopeHueristicScore = getNegativeSlopeHueristicScore(columnPlayed, gameBoard, isUserCoin);
							break;

						}

					}
				}
				);

			}
			else {
				horizontalHueristicScore = getHorizontalHueristicScore(columnPlayed, gameBoard, isUserCoin);
				verticalHueristicScore = getVerticalHueristicScore(columnPlayed, gameBoard, isUserCoin);
				positiveSlopeHueristicScore = getPositiveSlopeHueristicScore(columnPlayed, gameBoard, isUserCoin);
				negativeSlopeHueristicScore = getNegativeSlopeHueristicScore(columnPlayed, gameBoard, isUserCoin);
			}
		}

		//Check if the last play was a winning play
		bool wasWinningPlay(int columnPlayed, bool isUserCoin) {

			//Compute hueristic scores for horizontal, vertical and diagonal four coins in a row resulting from 
			//coin being dropped in column
			int horizontalHueristicScore, verticalHueristicScore, positiveSlopeHueristicScore, negativeSlopeHueristicScore;
			getHueristicScores(horizontalHueristicScore, verticalHueristicScore, positiveSlopeHueristicScore, negativeSlopeHueristicScore, columnPlayed, this->gameBoard, isUserCoin);
			//If it was a winning move, then return with indicator saying so
			if (horizontalHueristicScore == INT_MAX ||
				verticalHueristicScore == INT_MAX ||
				positiveSlopeHueristicScore == INT_MAX ||
				negativeSlopeHueristicScore == INT_MAX) {

				return true;
			}
			else {
				return false;
			}
		}

		//End the game
		void endTheGame(bool userWon) {			
			this->gameIsOver = true;
			this->userWonTheGame = userWon;
		}

		int bestHeuristicScoreForOpponentMoveParallel(int depth, bool isUserCoin, const model::GameBoard gameBoard) {

			//Do a map to find the move with the highest score
			std::vector<int> moveScores(gameBoard.getNumberOfColumns());

			tbb::parallel_for(
				tbb::blocked_range<int>(0, gameBoard.getNumberOfColumns()),
				[=, &moveScores](tbb::blocked_range<int> range) {

					for (int columnCounter = range.begin(); columnCounter != range.end(); ++columnCounter) {
						if (gameBoard.isValidPlay(columnCounter)) {
							model::GameBoard whatIfGameBoard{ gameBoard.getGameBoardVector(), this->gameBoard.getNumberOfRows(), this->gameBoard.getNumberOfColumns() };
							whatIfGameBoard.forceDropCoin(columnCounter, isUserCoin);
							moveScores.at(columnCounter) = getMoveHueristicScore(depth, columnCounter, isUserCoin, whatIfGameBoard);
						}
						else {
							moveScores.at(columnCounter) = -1 * INT_MAX;
						}
					}
				}
			);

			int bestScore = -1 * INT_MAX;
			for (int moveCounter = 0; moveCounter < gameBoard.getNumberOfColumns(); ++moveCounter) {
				if (moveScores.at(moveCounter) > bestScore) {
					bestScore = moveScores.at(moveCounter);
				}
			}

			return bestScore;

		}

		int bestHeuristicScoreForOpponentMoveSeries(int depth, bool isUserCoin, const model::GameBoard gameBoard) {

			int currentScore, bestScore = -1 * INT_MAX;
			for (int columnCounter = 0; columnCounter < gameBoard.getNumberOfColumns(); ++columnCounter) {

				if (gameBoard.isValidPlay(columnCounter)) {
					//Make a copy of the current gameboard to simulate a dropped coin
					model::GameBoard whatIfGameBoard{ gameBoard.getGameBoardVector(), this->gameBoard.getNumberOfRows(), this->gameBoard.getNumberOfColumns() };
					whatIfGameBoard.forceDropCoin(columnCounter, isUserCoin);

					currentScore = getMoveHueristicScore(depth, columnCounter, isUserCoin, whatIfGameBoard);
					if (currentScore > bestScore) {
						bestScore = currentScore;
					}

				}
			}

			return bestScore;
		}

		//Compute best heuristic score for opponent move
		int bestHeuristicScoreForOpponentMove(int depth, bool isUserCoin, const model::GameBoard gameBoard) {

			if (this->gameModeIsParallel) {
				return bestHeuristicScoreForOpponentMoveParallel(depth, isUserCoin, gameBoard);
			}
			else {
				return bestHeuristicScoreForOpponentMoveSeries(depth, isUserCoin, gameBoard);
			}

		}

		//Compute and return the hueristic score for the move
		int getMoveHueristicScore(int depth, int columnPlayed, bool isUserCoin, model::GameBoard gameBoard) {

			//If maximum depth has been reached, then return
			if (depth == 0) {
				return 0;
			}

			//Compute hueristic scores for horizontal, vertical and diagonal four coins in a row resulting from 
			//coin being dropped in column
			int horizontalHueristicScore, verticalHueristicScore, positiveSlopeHueristicScore, negativeSlopeHueristicScore;
			getHueristicScores(horizontalHueristicScore, verticalHueristicScore, positiveSlopeHueristicScore, negativeSlopeHueristicScore, columnPlayed, gameBoard, isUserCoin);

			//If it was a winning move, then return with indicator saying so
			if (horizontalHueristicScore == INT_MAX ||
				verticalHueristicScore == INT_MAX ||
				positiveSlopeHueristicScore == INT_MAX ||
				negativeSlopeHueristicScore == INT_MAX) {

				return INT_MAX;
			}

			int heuristicScoreForCurrentMove = horizontalHueristicScore +
				verticalHueristicScore +
				positiveSlopeHueristicScore +
				negativeSlopeHueristicScore;

			int bestOpponentMoveScore = bestHeuristicScoreForOpponentMove(depth - 1, isUserCoin ? false : true, gameBoard);

			if (bestOpponentMoveScore == INT_MAX || bestOpponentMoveScore == -INT_MAX) {
				return -1 * bestOpponentMoveScore;
			}
			else {
				return heuristicScoreForCurrentMove - bestOpponentMoveScore;
			}
			 
		}

		//Check if the cells between from and to index are of the required type or empty. 
		//Also count the number of cells of the required type.
		bool isEmptyOrRequiredType(int fromIndex, int toIndex, bool userCoinPlayed, int& hueristicScore, model::GameBoard gameBoard) {

			int coinCount = 0, nextRow, nextColumn, nextIndex;
			bool firstTime = true;

			int fromRow = gameBoard.getRowNumber(fromIndex);
			int fromColumn = gameBoard.getColumnNumber(fromIndex);
			int toRow = gameBoard.getRowNumber(toIndex);
			int toColumn = gameBoard.getColumnNumber(toIndex);

			for (int counter = 0; counter <= COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW; ++counter) {

				if (fromRow == toRow) {//horizontal sequence
					nextRow = fromRow;
					nextColumn = fromColumn + counter;
					nextIndex = gameBoard.getBoardIndex(nextRow, nextColumn);
				}
				else if (fromColumn == toColumn) {//vertical sequence
					nextRow = fromRow + counter;
					nextColumn = fromColumn;
					nextIndex = gameBoard.getBoardIndex(nextRow, nextColumn);
				}
				else if (fromRow > toRow && fromColumn < toColumn) {//diagonal going up
					if (firstTime) {
						firstTime = false;
						nextIndex = fromIndex;
					}
					else {
						nextIndex = gameBoard.getDiagonalCellToRightGoingUp(nextIndex);
					}

				}
				else if (fromRow < toRow && fromColumn < toColumn) {//diagonal going down
					if (firstTime) {
						firstTime = false;
						nextIndex = fromIndex;
					}
					else {
						nextIndex = gameBoard.getDiagonalCellToRightGoingDown(nextIndex);
					}

				}

				if (gameBoard.getGameSlot(nextIndex).hasComputerCoin()) {
					if (userCoinPlayed) {
						return false;
					}
					else {
						++coinCount;
					}
				}
				else if (gameBoard.getGameSlot(nextIndex).hasUserCoin()) {
					if (userCoinPlayed) {
						++coinCount;
					}
					else {
						return false;
					}
				}

			}

			if (coinCount == 1) {
				hueristicScore = HEURISTIC_SCORE_FOR_ONE_IN_ROW;
			}
			else if (coinCount == 2) {
				hueristicScore = HEURISTIC_SCORE_FOR_TWO_IN_ROW;
			}
			else if (coinCount == 3) {
				hueristicScore = HEURISTIC_SCORE_FOR_THREE_IN_ROW;
			}
			else if (coinCount == 4) {
				hueristicScore = HEURISTIC_SCORE_FOR_FOUR_IN_ROW;
			}
			else {
				hueristicScore = 0;
			}

			return true;
		}

		//Heuristic score for dropping a coin in the column played for all potential four-in-a-row horizontal configurations.
		int getHorizontalHueristicScore(int columnPlayed, model::GameBoard gameBoard, bool isUserCoin) {

			int slidingWindowStartPosition, hueristicScore, totalHueristicScore = 0, endColumn;
			if (columnPlayed >= COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW) {
				slidingWindowStartPosition = columnPlayed - COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
			}
			else {
				slidingWindowStartPosition = 0;
			}

			//Consider each four-in-a-row window starting from three before dropped column and ending at the dropped position
			for (int startColumn = slidingWindowStartPosition; startColumn <= columnPlayed; ++startColumn) {

				if (startColumn + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW > gameBoard.getNumberOfColumns() - 1) {
					break;
				}
				else {
					endColumn = startColumn + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
				}

				int rowContainingDroppedCoin = gameBoard.getRowNumber(gameBoard.getPlayedSlot(columnPlayed));
				if (isEmptyOrRequiredType(gameBoard.getBoardIndex(rowContainingDroppedCoin, startColumn), gameBoard.getBoardIndex(rowContainingDroppedCoin, endColumn), isUserCoin, hueristicScore, gameBoard)) {
					if (hueristicScore == INT_MAX) {
						return INT_MAX;
					}
					else {
						totalHueristicScore += hueristicScore;
					}
				}
			}

			return totalHueristicScore;
		}

		//Heuristic score for dropping a coin in the column played for all potential four-in-a-row vertical configurations.
		int getVerticalHueristicScore(int columnPlayed, model::GameBoard gameBoard, bool isUserCoin) {

			int slidingWindowStartPosition, hueristicScore, totalHueristicScore = 0, endRow;
			int coinDroppedInRow = gameBoard.getRowNumber(gameBoard.getPlayedSlot(columnPlayed));

			if (coinDroppedInRow >= COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW) {
				slidingWindowStartPosition = coinDroppedInRow - COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
			}
			else {
				slidingWindowStartPosition = 0;
			}

			//Consider each four-in-a-row window in the vertical column
			for (int startRow = slidingWindowStartPosition; startRow <= coinDroppedInRow; ++startRow) {

				if (startRow + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW > gameBoard.getNumberOfRows() - 1) {
					break;
				}
				else {
					endRow = startRow + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
				}

				if (isEmptyOrRequiredType(gameBoard.getBoardIndex(startRow, columnPlayed), gameBoard.getBoardIndex(endRow, columnPlayed), isUserCoin, hueristicScore, gameBoard)) {
					if (hueristicScore == INT_MAX) {
						return INT_MAX;
					}
					else {
						totalHueristicScore += hueristicScore;
					}
				}
			}

			return totalHueristicScore;
		}

		int getPositiveSlopeHueristicScore(int columnPlayed, model::GameBoard gameBoard, bool isUserCoin) {

			int slidingWindowStartRowPosition, slidingWindowStartColumnPosition, hueristicScore, totalHueristicScore = 0, endRow, endColumn;
			int coinDroppedInRow = gameBoard.getRowNumber(gameBoard.getPlayedSlot(columnPlayed));

			if (gameBoard.getNumberOfRows() - 1 - coinDroppedInRow >= COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW && columnPlayed >= COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW) {
				//If you can go three down and left to start
				slidingWindowStartRowPosition = coinDroppedInRow + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
				slidingWindowStartColumnPosition = columnPlayed - COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
			}
			else {
				//You can only got the minimum of two values down and left
				int rowDistanceFromBottom = gameBoard.getNumberOfRows() - 1 - coinDroppedInRow;
				int columnDistanceFromLeft = columnPlayed;

				slidingWindowStartRowPosition = coinDroppedInRow + std::min(rowDistanceFromBottom, columnDistanceFromLeft);
				slidingWindowStartColumnPosition = columnPlayed - std::min(rowDistanceFromBottom, columnDistanceFromLeft);
			}

			//Consider each four-in-a-row window starting from three before dropped column and ending at the dropped position
			for (int startRow = slidingWindowStartRowPosition, startColumn = slidingWindowStartColumnPosition; startColumn <= columnPlayed; --startRow, ++startColumn) {

				if (startColumn + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW > gameBoard.getNumberOfColumns() - 1 ||
					startRow < COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW) {
					break;
				}
				else {
					endRow = startRow - COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
					endColumn = startColumn + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
				}

				if (isEmptyOrRequiredType(gameBoard.getBoardIndex(startRow, startColumn), gameBoard.getBoardIndex(endRow, endColumn), isUserCoin, hueristicScore, gameBoard)) {
					if (hueristicScore == INT_MAX) {
						return INT_MAX;
					}
					else {
						totalHueristicScore += hueristicScore;
					}
				}
			}

			return totalHueristicScore;
		}

		int getNegativeSlopeHueristicScore(int columnPlayed, model::GameBoard gameBoard, bool isUserCoin) {

			int slidingWindowStartRowPosition, slidingWindowStartColumnPosition, hueristicScore, totalHueristicScore = 0, endRow, endColumn;
			int coinDroppedInRow = gameBoard.getRowNumber(gameBoard.getPlayedSlot(columnPlayed));

			if (coinDroppedInRow >= COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW && columnPlayed >= COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW) {
				slidingWindowStartRowPosition = coinDroppedInRow - COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
				slidingWindowStartColumnPosition = columnPlayed - COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
			}
			else {
				int rowDistanceFromTop = coinDroppedInRow;
				int columnDistanceFromLeft = columnPlayed;

				slidingWindowStartRowPosition = coinDroppedInRow - std::min(rowDistanceFromTop, columnDistanceFromLeft);
				slidingWindowStartColumnPosition = columnPlayed - std::min(rowDistanceFromTop, columnDistanceFromLeft);
			}

			//Consider each four-in-a-row window starting from three before dropped column and ending at the dropped position
			for (int startRow = slidingWindowStartRowPosition, startColumn = slidingWindowStartColumnPosition; startColumn <= columnPlayed; ++startRow, ++startColumn) {

				if (startColumn + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW > gameBoard.getNumberOfColumns() - 1 ||
					startRow + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW > gameBoard.getNumberOfRows() - 1) {
					break;
				}
				else {
					endRow = startRow + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
					endColumn = startColumn + COLUMN_OR_ROW_DIFFERENCE_FOR_FOUR_IN_A_ROW;
				}

				if (isEmptyOrRequiredType(gameBoard.getBoardIndex(startRow, startColumn), gameBoard.getBoardIndex(endRow, endColumn), isUserCoin, hueristicScore, gameBoard)) {
					if (hueristicScore == INT_MAX) {
						return INT_MAX;
					}
					else {
						totalHueristicScore += hueristicScore;
					}
				}
			}

			return totalHueristicScore;
		}

		//Find best move by considering all columns in parallel using the Map pattern
		int evaluatePotentialMovesInParallel() {

			int depth = this->gameDifficultyLevel;
			std::vector<int> moveScores(this->gameBoard.getNumberOfColumns());

			tbb::parallel_for(
				tbb::blocked_range<int>(0, this->gameBoard.getNumberOfColumns()),
				[=, &moveScores](tbb::blocked_range<int> range) {

				for (int columnCounter = range.begin(); columnCounter != range.end(); ++columnCounter) {
					if (this->gameBoard.isValidPlay(columnCounter)) {

						//Make a copy of the current gameboard to simulate a dropped coin
						model::GameBoard whatIfGameBoard{ this->gameBoard.getGameBoardVector(), this->gameBoard.getNumberOfRows(), this->gameBoard.getNumberOfColumns() };
						whatIfGameBoard.forceDropCoin(columnCounter, false);
						moveScores.at(columnCounter) = getMoveHueristicScore(depth, columnCounter, false, whatIfGameBoard);

					}
				}
			}
			);

			int bestMove = 0, bestScore = -1 * INT_MAX;
			for (int moveCounter = 0; moveCounter < this->gameBoard.getNumberOfColumns(); ++moveCounter) {
				if (moveScores.at(moveCounter) > bestScore) {
					bestScore = moveScores.at(moveCounter);
					bestMove = moveCounter;
				}
			}

			return bestMove;

		}

		//Find best move by considering all columns in one after the other
		int evaluatePotentialMovesInSeries() {

			int depth = this->gameDifficultyLevel, currentScore, bestScore = -1 * INT_MAX, bestMove = 0;
			for (int columnCounter = 0; columnCounter < this->gameBoard.getNumberOfColumns(); ++columnCounter) {

				if (this->gameBoard.isValidPlay(columnCounter)) {
					//Make a copy of the current gameboard to simulate a dropped coin
					model::GameBoard whatIfGameBoard(this->gameBoard.getGameBoardVector(), this->gameBoard.getNumberOfRows(), this->gameBoard.getNumberOfColumns());
					whatIfGameBoard.forceDropCoin(columnCounter, false);
					currentScore = getMoveHueristicScore(depth, columnCounter, false, whatIfGameBoard);

					if (currentScore > bestScore) {
						bestScore = currentScore;
						bestMove = columnCounter;
					}
				}
			}
			return bestMove;
		}

		//Consider all possible moves and play the one with the best hueristic score that maximizes the chance of winning 
		int counterUserMove() {
			if (this->gameModeIsParallel) {
				return evaluatePotentialMovesInParallel();
			}
			else {
				return evaluatePotentialMovesInSeries();
			}
		}

		//Drop a coin into one of the columns
		void dropCoin(int dropInColumn, bool isUserCoin) {

			//First check if the play is a valid one
			if (this->gameBoard.isValidPlay(dropInColumn)) {

				//Place a user coin in the top available position
				GameSlot& gameSlot = this->gameBoard.getGameSlot(this->gameBoard.getAvailableSlot(dropInColumn));
				gameSlot.putCoin(isUserCoin);

				if (wasWinningPlay(dropInColumn, true) || isGameBoardFull()) {
					endTheGame(true);
				}
				else {
					//Make a move to best counter the user move
					int columnToPlay = counterUserMove();
					//std::cout << "User move countered by dropping in column " << columnToPlay << std::endl;
					int rowToPlay = this->gameBoard.getRowNumber(this->gameBoard.getAvailableSlot(columnToPlay));
					GameSlot& gameSlot = this->gameBoard.getGameSlot(this->gameBoard.getBoardIndex(rowToPlay, columnToPlay));
					gameSlot.putCoin(false);
					if (wasWinningPlay(columnToPlay, false) || isGameBoardFull()) {
						endTheGame(false);
					}
				}
			}
		}


	public:

		//Default constructor will set game parameters using default values
		ConnectFourGame() {

			gameBoard = model::GameBoard();
			this->firstPlayerIsUser = DEFAULT_FIRST_PLAYER_IS_USER;
			this->gameDifficultyLevel = DEFAULT_DIFFICULTY_LEVEL;
			this->gameModeIsParallel = DEFAULT_MODE_IS_PARALLEL;
			this->gameIsOver = false;

		}

		ConnectFourGame(int numberOfRows, int numberOfColumns) {

			gameBoard = model::GameBoard(numberOfRows, numberOfColumns);
			this->firstPlayerIsUser = DEFAULT_FIRST_PLAYER_IS_USER;
			this->gameDifficultyLevel = DEFAULT_DIFFICULTY_LEVEL;
			this->gameModeIsParallel = DEFAULT_MODE_IS_PARALLEL;
			this->gameIsOver = false;

		}

		//First player will be determined by user selection
		void setWhoPlaysFirst(bool firstPlayerIsUser) {
			this->firstPlayerIsUser = firstPlayerIsUser;
		}

		//Difficulty level will be set by user
		void setgameDifficultyLevel(int gameDifficultyLevel) {
			this->gameDifficultyLevel = gameDifficultyLevel;
		}

		bool isGameOver() {
			return this->gameIsOver;
		}

		bool didUserWinTheGame() {
			return this->userWonTheGame;
		}

		//Set the serial/parallel computation mode depending on user preference
		void setComputationModeToParallel(bool parallelMode) {
			this->gameModeIsParallel = parallelMode;
		}

		const model::GameBoard& getGameBoard() const {
			return this->gameBoard;
		}

		//User method to drop a coin into one of the columns
		void dropCoin(int dropInColumn) {

			if (!this->gameIsOver) {
				dropCoin(dropInColumn, true);
			}
		}

		//Method to check if the board has been filled and play cannot continue
		bool isGameBoardFull() {

			//Check to see that at least one slot in the top row is empty
			for (int counter = 0; counter < this->gameBoard.getNumberOfColumns(); ++counter) {
				if (this->gameBoard.getGameSlot(counter).isEmpty()) {
					return false;
				}
			}

			return true;
		}
	};

}

#endif