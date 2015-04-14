#include "ConnectFourGame.hpp"
#include "GameBoard.hpp"

#include <iostream>
#include <string>

int getColumnPlayedByUser() {

	int columnSelectedByUser;
	std::cout << "Which column do you want to drop the coin in (1 to 7): ";
	while (true) {
		std::cin >> columnSelectedByUser;
		if (columnSelectedByUser >= 1 && columnSelectedByUser <= 7) {
			return columnSelectedByUser - 1;
		}
	}
}

//Show the game board to the user
void showGameBoard(const std::vector<GameSlot>& gameBoardVector, int numberOfRowsToDisplay, int numberOfColumnsToDisplay) {

	for (int rowCounter = 0; rowCounter < numberOfRowsToDisplay; ++rowCounter) {
		for (int columnCounter = 0; columnCounter < numberOfColumnsToDisplay; ++columnCounter) {

			GameSlot gameSlot = gameBoardVector.at(rowCounter * numberOfColumnsToDisplay + columnCounter);
			if (gameSlot.isEmpty()) {
				std::cout << "_";
			}
			else if (gameSlot.hasUserCoin()) {
				std::cout << "U";
			}
			else {
				std::cout << "C";
			}
		}
		std::cout << std::endl;
	}

}

int main() {

	//Create a connect four game with default parameters
	controller::ConnectFourGame connectFourGame{};

	//Set difficulty level
	int difficultyLevel;
	std::cout << "What difficulty level do you want (1 to 10): ";
	std::cin >> difficultyLevel;
	if (difficultyLevel >= 1 && difficultyLevel <= 10) {
		connectFourGame.setgameDifficultyLevel(difficultyLevel);
	}
	else {
		std::cout << std::endl << "Difficulty level set to default value" << std::endl;
	}

	std::string computationsInParallel;
	std::cout << "Do you want to run parallel computations? ";
	std::cin >> computationsInParallel;
	if (computationsInParallel.compare("N") == 0 || computationsInParallel.compare("n") == 0) {
		connectFourGame.setComputationModeToParallel(false);
	}
	else {
		std::cout << std::endl << "Computations will be in parallel" << std::endl;
	}

	model::GameBoard gameBoard;

	while (true) {

		//Drop a coin into the column selected by the user
		connectFourGame.dropCoin(getColumnPlayedByUser());
		gameBoard = connectFourGame.getGameBoard();
		showGameBoard(gameBoard.getGameBoardVector(), gameBoard.getNumberOfRows(), gameBoard.getNumberOfColumns());
		if (connectFourGame.isGameOver()) {
			break;
		}
	}

	
	connectFourGame.dropCoin(3);

}