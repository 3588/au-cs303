#include <iostream>

using namespace std;
void merge(double*, int, int, int);
void mergeSort(double*, int, int);


int main()
{
	/*
8
-64.22 -30.06 -19.54 86.72 -16.44 15.12 54.60 78.56

10
-51.02 40.06 75.90 8.52 76.58 36.52 61.74 -68.30 45.96 -64.06
*/
	int size1 = 8;
	double s1[] = { -64.22,-30.06,-19.54,86.72,-16.44,15.12,54.60,78.56};

	int size2 = 10;
	double s2[] = {-51.02,40.06,75.90,8.52,76.58,36.52,61.74,-68.30,45.96,-64.06};

	cout << "old1: ";
	for (int i = 0; i<size1; i++)
		cout << s1[i] << " ";
	cout << endl;
	mergeSort(s1, 0, size1 - 1);

	cout << "new1: ";
	for (int i = 0; i<size1; i++)
		cout << s1[i] << " ";
	cout << endl;


	cout << "old2: ";
	for (int i = 0; i<size2; i++)
		cout << s2[i] << " " ;
	cout << endl;
	mergeSort(s2, 0, size2 - 1);

	cout << "new2: ";
	for (int i = 0; i<size2; i++)
		cout << s2[i] << " " ;
	cout << endl;

	int x;
	cin >> x;
	return 0;
}
void merge(double *s, int m, int k, int n)
{
	int sizePart1 = k - m + 1;
	int sizePart2 = n - k;
	int size = sizePart1 + sizePart2;
	double *c = (double*)malloc(sizeof(double)*size);

	int ia = m;
	int ib = k + 1;
	double tmpA;
	double tmpB;
	for (int i = 0; i<size; i++)
	{
		if (ia > k)
			tmpA = INT_MAX;
		else
			tmpA = s[ia];
		if (ib > n)
			tmpB = INT_MAX;
		else
			tmpB = s[ib];

		if (tmpA <= tmpB)
		{
			c[i] = s[ia];
			ia++;
		}
		else
		{
			c[i] = s[ib];
			ib++;
		}
	}

	memcpy(s + m, c, sizeof(double)*size);
	free(c);
}

void mergeSort(double *s, int m, int n)
{
	if (m == n)
		return;
	int mid = (n - m) / 2 + m;
	mergeSort(s, m, mid);
	mergeSort(s, mid + 1, n);
	merge(s, m, mid, n);
}