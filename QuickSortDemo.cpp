#include "sorting.hpp"

//Ask user to enter the number of elements to sort
int getNumberOfElementsToSort() {

	int numberOfElementsToSort;
	std::string userResponse;

	std::cout << "How many elements do you want to sort? ";
	std::cin >> userResponse;

	try {
		numberOfElementsToSort = std::stoi(userResponse);
	}
	catch (std::invalid_argument&)  {
		std::cout << "Could not parse " << userResponse << " as an integer." << std::endl;
		exit(0);
	}

	return numberOfElementsToSort;
}

int main() {

	//Get user preference for seeing data in standard output
	bool showData = false;
	std::cout << "Do you want to see data before and after sorting? Answer Y or N: ";
	std::string userResponse;
	std::cin >> userResponse;
	std::ofstream outputFile;

	if (userResponse.compare("Y") == 0 || userResponse.compare("y") == 0) {
		showData = true;
		outputFile.open("outputFile.txt");
	}

	//Get user preference on number of data elements to sort
	int numberOfElementsToSort = getNumberOfElementsToSort();

	//Do serial sorting 
	SortableCollection sortableCollectionForSerial(numberOfElementsToSort);

	if (showData) {
		std::vector<int> dataToBeSerialSorted(sortableCollectionForSerial.getData());
		outputFile << "This is the sequential input data:" << std::endl;
		for (int input : dataToBeSerialSorted) {
			outputFile << input << " " << std::endl;
		}
	}

	sortableCollectionForSerial.doSequentialSort();

	if (showData) {
		outputFile << "This is the sequential sorted data:" << std::endl;
		std::vector<int> serialSortedData = sortableCollectionForSerial.getData();
		for (std::vector<int>::iterator it = serialSortedData.begin(); it != serialSortedData.end(); ++it) {
			outputFile << *it << " " << std::endl;
		}
	}

	std::cout << "Sequential sorting took " << sortableCollectionForSerial.getRunningTime() << " milliseconds" << std::endl;

	//Do parallel sorting
	SortableCollection sortableCollectionForParallel(numberOfElementsToSort);

	if (showData) {
		std::vector<int> dataToBeSortedInParallel(sortableCollectionForParallel.getData());
		outputFile << "This is the parallel input data:" << std::endl;
		for (int input : dataToBeSortedInParallel) {
			outputFile << input << " " << std::endl;
		}
	}

	sortableCollectionForParallel.doParallelSort();

	if (showData) {
		outputFile << "This is the parallel sorted data:" << std::endl;
		std::vector<int> parallelSortedData = sortableCollectionForParallel.getData();

		for (std::vector<int>::iterator it = parallelSortedData.begin(); it != parallelSortedData.end(); ++it) {
			outputFile << *it << " " << std::endl;
		}
		outputFile.close();
	}

	std::cout << "Parallel sorting took " << sortableCollectionForParallel.getRunningTime() << " milliseconds" << std::endl;
}