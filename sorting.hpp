#include <tbb\task_group.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>


class SortableCollection {

private:

	//Threshold for parallel sorting
	ptrdiff_t PARALLEL_SORT_THESHOLD = 500;

	//Running time for the last sort operation
	double runningTime;

	//Input data to be sorted
	std::vector<int> inputData;

	////Sorted data
	std::vector<int> sortedData;

	//Choose median of three keys from values to be sorted
	int* medianOfThree(int* x, int* y, int* z) {

		return *x < *y ? *y < *z ? y : *x < *z ? z : x : *z < *y ? y : *z < *x ? z : x;

	}

	//Choose a partition as median of medians
	int* choosePartitionKey(int* first, int* last) {

		size_t offset = (last - first) / 8;

		return medianOfThree(medianOfThree(first, first + offset, first + offset * 2),
			medianOfThree(first + offset * 3, first + offset * 4, last - (3 * offset + 1)),
			medianOfThree(last - (2 * offset + 1), last - (offset + 1), last - 1));

	}

	//Partition and return the position of the key
	int* divide(int* first, int* last) {

		//Move the partition key to the front of the array
		std::swap(*first, *choosePartitionKey(first, last));

		//Partition the array
		int key = *first;
		int* middle = std::partition(first + 1, last, [=](const int& data) {return data < key; }) - 1;

		if (middle != first) {
			//Move the key between the two partitions
			std::swap(*first, *middle);
		}
		else {
			//Return null if all keys are equal since there is no need to sort
			if (last == std::find_if(first + 1, last, [=](const int& data){return key < data; })) {
				return nullptr;
			}
		}
		return middle;
	}

	void parallelQuickSort(int* firstElement, int* lastElement) {

		tbb::task_group parallelSortGroup;

		//Do parallel sort for larger data size
		while (lastElement - firstElement > PARALLEL_SORT_THESHOLD) {

			//Partition the array
			int* middleElement = divide(firstElement, lastElement);
			//If all elements are same, no more partitioning is required
			if (middleElement == nullptr) {
				parallelSortGroup.wait();
				return;
			}

			//The array has now been partitioned into two
			if (middleElement - firstElement < lastElement - (middleElement + 1)) {

				//The left partition is smaller and so spawn its sort
				parallelSortGroup.run([=]{parallelQuickSort(firstElement, middleElement); });

				//The next iteration will sort the right part of the array
				firstElement = middleElement + 1;

			}
			else {

				//The right partition is smaller and so spawn its sort
				parallelSortGroup.run([=]{parallelQuickSort(middleElement + 1, lastElement); });

				//The next iteration will sort the left part of the array
				lastElement = middleElement;
			}

		}

		//Number of elements is below the parallel threshold. So do serial sort.
		std::sort(firstElement, lastElement + 1);
		parallelSortGroup.wait();
	}

public:

	//Constructor having path and file name of input data as parameter 
	SortableCollection(std::string inputDataFilePath) {

		//Open the input file
		std::string inputLine, inputNumber;
		std::ifstream inputFile(inputDataFilePath);
		if (inputFile.is_open()) {

			//Read each line from input data text file
			while (getline(inputFile, inputLine)) {

				//Process the comma separated tokens
				std::istringstream inputStream(inputLine);
				while (std::getline(inputStream, inputNumber, ',')) {

					//Put the number to be sorted into the input vector
					try {
						inputData.push_back(std::stoi(inputNumber));
					}
					catch (std::invalid_argument&)  {
						std::cout << "Could not parse " << inputNumber << " as an integer." << std::endl;
						exit(0);
					}
				}

			}

			inputFile.close();

		}
		else {
			std::cout << "Could not open input data file " << inputDataFilePath << "." << std::endl;
			exit(0);
		}

	}

	//Constructor with number of input data elements to be generated as parameter
	SortableCollection(int dataSize) {

		//Random number generator with uniform distribution
		std::default_random_engine randomNumberGenerator;
		std::uniform_int_distribution<int> distribution(0, INT_MAX);

		//Fill input vector with random numbers to be sorted
		for (int dataSizeCounter = 0; dataSizeCounter < dataSize; ++dataSizeCounter) {
			inputData.push_back(distribution(randomNumberGenerator));
		}

	}

	//Do a sequential QuickSort on the input data and return the sorted result
	void doSequentialSort() {

		//Get current time before sorting
		auto start = std::chrono::high_resolution_clock::now();

		std::sort(this->inputData.begin(), this->inputData.end());
		
		//Find time spent in sorting
		std::chrono::milliseconds runTimeInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
		
		//Store the last run time in seconds
		this->runningTime = runTimeInMilliseconds.count() * 1.0;
	}

	std::vector<int> getData() {

		//Return a copy of the input data
		return std::vector<int>(this->inputData);
	}

	//Do a parallel QuickSort on the input data and return the sorted result
	void doParallelSort() {

		//Get current time before sorting
		auto start = std::chrono::high_resolution_clock::now();

		//Get internal array representation of the vector to be sorted
		int numberOfElements = this->inputData.size(); 
		int* elements = this->inputData.data();
		int* firstElement = &elements[0];
		int* lastElement = &elements[numberOfElements - 1];

		//Do the sorting in parallel
		parallelQuickSort(firstElement, lastElement);

		//Find time spent in sorting
		std::chrono::milliseconds runTimeInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);

		//Store the last run time in seconds
		this->runningTime = runTimeInMilliseconds.count() * 1.0;

	}

	//Return the run time of the last sort operation
	double getRunningTime() {

		return this->runningTime;
	}

};