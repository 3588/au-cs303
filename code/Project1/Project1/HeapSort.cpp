#include <iostream>
using namespace std;

void exchange(double &a, double &b)
{
	double temp = a;
	a = b;
	b = temp;
}

void siftdown(double nums[], int i, int size)
{
int c = 2*i+1;
if (c >= size)
{
return;
}

if ((c+1<size) && (nums[c+1] > nums[c]))
{
c++;
}
if (nums[c] > nums[i])
{
exchange(nums[c], nums[i]);
siftdown(nums, c, size);
}
}


void MakeHeap(double nums[], int size)
{


for (int i=size-1; i>=0; i--)
siftdown(nums, i, size);
}

void Display(double nums[], int size)
{
	for (int i = 0; i<size; i++)
	{
		cout << nums[i];
	}
	cout << endl;
}

void HeapSort(double nums[], int size)
{
	for (int i = size - 1; i >= 0; i--)
	{
		exchange(nums[i], nums[0]);     
		size--;                         
		MakeHeap(nums, size);          
	}
}

int main()
{
	double nums[11] = {-51.02,40.06,75.90,8.52,76.58,36.52,61.74,-68.30,45.96,-64.06};

	int size = sizeof(nums) / sizeof(double);

	MakeHeap(nums, size);

	HeapSort(nums, size);

	Display(nums, size);

	cin >> size;

	return 0;
}