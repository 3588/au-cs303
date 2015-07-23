#include <iostream>  
#include <ctime>  
#include <cstdlib>  

using namespace std;

void quickSort(double*, int, int);
int partition(double*, int, int);
void exchange(double &a, double &b);

int main()
{
	int size1 = 8;
	double s1[] = { -64.22, -30.06, -19.54, 86.72, -16.44, 15.12, 54.60, 78.56 };

	int size2 = 10;
	double s2[] = { -51.02, 40.06, 75.90, 8.52, 76.58, 36.52, 61.74, -68.30, 45.96, -64.06 };
	
	cout << "old1: ";
	for (int i = 0; i<size1; i++)
		cout << s1[i] << " ";
	cout << endl;

	quickSort(s1, 0, size1 - 1);

	cout << "new1: ";
	for (int i = 0; i<size1; i++)
		cout << s1[i] << " ";
	cout << endl;

	cout << "old2: ";
	for (int i = 0; i<size2; i++)
		cout << s2[i] << " ";
	cout << endl;

	quickSort(s2, 0, size2 - 1);

	cout << "new2: ";
	for (int i = 0; i<size2; i++)
		cout << s2[i] << " ";
	cout << endl;


	return 0;
}//main  

void quickSort(double * data, int low, int high)
{
	if (low < high)
	{
		int temp = partition(data, low, high);
		quickSort(data, low, temp - 1);
		quickSort(data, temp + 1, high);
	}
}

int partition(double * data, int low, int high)
{
	int i = low - 1;
	double x = data[high];

	for (int j = low; j<high; j++)
	{
		if (data[j] <= x) 
		{
			i += 1;
			exchange(data[i], data[j]);
		}
	}
	exchange(data[i + 1], data[high]);
	return i + 1;  
}
void exchange(double &a, double &b)
{
	double temp = a;
	a = b;
	b = temp;
}