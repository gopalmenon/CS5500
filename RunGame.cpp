#include "ConnectFourGame.hpp"

int main() {

	//Create a connect four game with default parameters
	ConnectFourGame connectFourGame{};

	//Drop a coin into the middle column
	connectFourGame.dropCoin(3);

}