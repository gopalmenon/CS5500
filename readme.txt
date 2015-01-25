Description of Work Done:

I had some confusion on the understanding of the Map pattern and proposed doing a parallel QuickSort with the sorting of the data to the left and right of the pivot element being done in parallel. The proposal got accepted with the comment that I needed to use the Fork-Join pattern and that the book included an example of such a sort.

So my code is heavily influenced by the code in the book. Had I known the book had QuickSort, I would have chosen something else. 

Here are the run time statistics for the serial and parallel sorts.

Elements     Serial Sort (ms)    Parallel Sort (ms)      Speedup
10                          0                 0.003        0.000
100                         0                 0.002        0.000
1000                        0                 0.002        0.000        
10000                   0.002                 0.005        0.400 
100000                  0.033                 0.014        2.357
1000000                 0.108                 0.048        2.250
10000000                1.315                 0.415        3.169

I did not get sufficient time to make a GUI for this. And so this is a command line application. I have not covered the comma separated input data as without a GUI, it was cumbersome to choose between randomly generated data and importing comma separated data for sorting.

Here is what the running application user interaction looks like:

Do you want to see data before and after sorting? Answer Y or N: n
How many elements do you want to sort? 10000000
Sequential sorting took 1.315 milliseconds
Parallel sorting took 0.415 milliseconds

If the user answers Y to the first question, the program puts the input unsorted data and the sorted data into an outfile.