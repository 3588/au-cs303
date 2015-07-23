#include <iostream>
using namespace std;
int main(){
	/*
	8
	-64.22 -30.06 -19.54 86.72 -16.44 15.12 54.60 78.56

	10
	-51.02 40.06 75.90 8.52 76.58 36.52 61.74 -68.30 45.96 -64.06
	*/
	int size1 = 8;
	double s1[] = { -64.22, -30.06, -19.54, 86.72, -16.44, 15.12, 54.60, 78.56 };

	int size2 = 10;
	double s2[] = { -51.02, 40.06, 75.90, 8.52, 76.58, 36.52, 61.74, -68.30, 45.96, -64.06 };

	cout << "old1: ";
	for (int i = 0; i<size1; i++)
		cout << s1[i] << " ";
	cout << endl;
		
	for (int i = 0; i<size1 - 1; i++){
		for (int j = 0; j<size1 - 1; j++){
			if (s1[j]>s1[j + 1]){
				double temp = s1[j + 1];
				s1[j + 1] = s1[j];
				s1[j] = temp;
			}
		}
	}
	
		cout << "new1: ";
	for (int i = 0; i<size1; i++)
		cout << s1[i] << " ";
	cout << endl;


	cout << "old2: ";
	for (int i = 0; i<size2; i++)
		cout << s2[i] << " ";
	cout << endl;

	for (int i = 0; i<size2 - 1; i++){
		for (int j = 0; j<size2 - 1; j++){
			if (s2[j]>s2[j + 1]){
				double temp = s2[j + 1];
				s2[j + 1] = s2[j];
				s2[j] = temp;
			}
		}
	}

	cout << "new2: ";
	for (int i = 0; i<size2; i++)
		cout << s2[i] << " ";
	cout << endl;

	cin >> size1;
	return 0;
}