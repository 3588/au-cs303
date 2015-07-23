/*---------------------------------------------------------------
*
*   main.cpp for nw program.
*
*   Implements the Needleman-Wunsch algorithm
*   for global nucleotide sequence alignment.
*
*   Rolf Muertter,  rolf@dslextreme.com
*   9/5/2006
*
---------------------------------------------------------------*/


#include "nw.h"

using namespace std;


int  main()
{

	bool    prm = true;
	// Sequences to be aligned
	string  seq_1 = "AAAC";
	string  seq_2 = "AGC";

	// Aligned sequences
	string  seq_1_al;
	string  seq_2_al;

	// Get alignment
	nw(seq_1, seq_2, seq_1_al, seq_2_al, prm);

	// Print alignment
	print_al(seq_1_al, seq_2_al);
	return  0;

}
