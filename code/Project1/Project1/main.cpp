/*
I affirm that I have adhered to the Ashland University Honor code on this program
Junjun Huang
*/
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include<cstdlib>
#include<sstream>
#include <vector>
#include <time.h>
using namespace std;
char* OpenFileName = "new_data.txt";
int ReadFileTotalLineNum(char*);
vector<string> split(string, string);//This function from the internet
void SaveToFile(char*, double*, int*, double*);
/*Sort Function Start*/
void merge(double*, int, int, int);
void mergeSort(double*, int, int);
void bubblesort(double*, int);
void quickSort(double*, int, int);
int partition(double*, int, int);
void siftdown(double* , int, int);
void HeapSort(double*, int );
void exchange(double &a, double &b);

int main()
{
	/*File Read Part Start*/
	int *NumData = new int[ReadFileTotalLineNum(OpenFileName)];
	string *LineData = new string[ReadFileTotalLineNum(OpenFileName)];
	fstream file;
	file.open(OpenFileName, ios::in);
	string tempstr;
	int tempint1 = 0;
	while (!file.eof()){
		getline(file, tempstr);
		NumData[tempint1] = atof(const_cast<const char *>(tempstr.c_str()));
		getline(file, tempstr);
		LineData[tempint1] = tempstr;
		getline(file, tempstr);
		tempint1++;
	}
	int TotalData = 0;
	for (int i = 0; i < ReadFileTotalLineNum(OpenFileName); i++){
		TotalData = TotalData + NumData[i];
	}
	double *Data = new double[TotalData];
	TotalData = 0;// temp use
	for (int i = 0; i < ReadFileTotalLineNum(OpenFileName); i++){
		vector<string> result = split(LineData[i], " ");
		for (int j = 0; j<result.size(); j++){
			Data[TotalData] = atof(const_cast<const char *>(result[j].c_str()));
			TotalData++;
		}
	}
	file.close();
	/*File Read Part End*/
	/*Sort Part*/
	double *mergesortData = Data;
	double *bubblesortData = Data;
	double *quicksortData = Data;
	double *heapsortData = Data;
	double *mergesortDif = new double[ReadFileTotalLineNum(OpenFileName)];
	double *bubblesortDif = new double[ReadFileTotalLineNum(OpenFileName)];
	double *quicksortDif = new double[ReadFileTotalLineNum(OpenFileName)];
	double *heapsortDif = new double[ReadFileTotalLineNum(OpenFileName)];
	time_t start0, end0, start1, end1, start2, end2, start3, end3;
	int point = 0;
	for (int i = 0; i < ReadFileTotalLineNum(OpenFileName); i++){
		double *TempmergesortData = new double[NumData[i]];
		double *TempbubblesortData = new double[NumData[i]];
		double *TempquicksortData = new double[NumData[i]];
		double *TempheapsortData = new double[NumData[i]];
		for (int j = 0; j < NumData[i]; j++){
			TempmergesortData[j] = Data[point];
			TempbubblesortData[j] = Data[point];
			TempquicksortData[j] = Data[point];
			TempheapsortData[j] = Data[point];
			point++;
		}
		time(&start0);
		mergeSort(TempmergesortData,0,NumData[i] - 1);
		time(&end0);
		mergesortDif[i] = difftime(end0, start0);
		time(&start1);
		bubblesort(TempbubblesortData, NumData[i] - 1);
		time(&end1);
		bubblesortDif[i] = difftime(end1, start1);
		time(&start2);
		quickSort(TempquicksortData,0,NumData[i] - 1);
		time(&end2);
		quicksortDif[i] = difftime(end2, start2);
		time(&start3);
		quickSort(TempheapsortData, 0, NumData[i] - 1);
		time(&end3);
		heapsortDif[i] = difftime(end3, start3);
		int temppoint = point-1;
		for (int j = NumData[i] - 1; j > -1; j--){
			Data[temppoint] = TempmergesortData[j];
			Data[temppoint] = TempbubblesortData[j];
			Data[temppoint] = TempquicksortData[j];
			Data[temppoint] = TempheapsortData[j];
			temppoint--;
		}
	
	}
	SaveToFile("mergeSort_results.txt", mergesortData, NumData, mergesortDif);
	SaveToFile("bubblesort_results.txt", bubblesortData, NumData, bubblesortDif);
	SaveToFile("quicksort_results.txt", quicksortData, NumData, quicksortDif);
	SaveToFile("heapsort_results.txt", heapsortData, NumData, heapsortDif);
	cout << "DONE\n";
	int stop;
	cin >> stop;
	return 0;
}
/*The Function Reference By
http://www.cnblogs.com/MikeZhang/archive/2012/03/24/MySplitFunCPP.html
*/
vector<string> split(string str, string pattern){
	string::size_type pos;
	vector<std::string> result;
	str += pattern;
	int size = str.size();
	for (int i = 0; i<size; i++){
        pos = str.find(pattern, i);
		if (pos<size){
			string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}
int ReadFileTotalLineNum(char* FileName){
	ifstream file;
	int Num = 0;
	string temp;
	file.open(FileName, ios::in);
	while (getline(file, temp)){
		Num++;
	}
	return (Num+1)/3;
}
void SaveToFile(char* FileName, double *sData, int *m, double *dif){
	ofstream infile;
	infile.open(FileName, ios::trunc);
	int point = 0;
	for (int i = 0; i < ReadFileTotalLineNum(OpenFileName); i++){
		infile << m[i] << "\n";

		for (int j = 0; j<m[i]; j++){
			infile << sData[point] << " ";
			point++;
		}
		infile << "\nTime Speed: " << dif[i] << "\n\n";
	}
	infile.close();
}
void merge(double *sData, int m, int k, int n){
	int sizePart1 = k - m + 1;
	int sizePart2 = n - k;
	int size = sizePart1 + sizePart2;
	double *c = (double*)malloc(sizeof(double)*size);
	int ia = m;
	int ib = k + 1;
	double tmpA;
	double tmpB;
	for (int i = 0; i<size; i++){
		if (ia > k) tmpA = INT_MAX;
		else tmpA = sData[ia];
		if (ib > n) tmpB = INT_MAX;
		else tmpB = sData[ib];

		if (tmpA <= tmpB){
			c[i] = sData[ia];
			ia++;
		}
		else{
			c[i] = sData[ib];
			ib++;
		}
	}
	memcpy(sData + m, c, sizeof(double)*size);
	free(c);
}

void mergeSort(double *sData, int m, int n){
	if (m == n) return;
	int mid = (n - m) / 2 + m;
	mergeSort(sData, m, mid);
	mergeSort(sData, mid + 1, n);
	merge(sData, m, mid, n);
}
void bubblesort(double* sData, int m){
	for (int i = 0; i<m; i++){
		for (int j = 0; j<m; j++){
			if (sData[j]>sData[j + 1]){
				double temp = sData[j + 1];
				sData[j + 1] = sData[j];
				sData[j] = temp;
			}
		}
	}
}
void quickSort(double * sData, int m, int n){
	if (m < n){
		int temp = partition(sData, m, n);
		quickSort(sData, m, temp - 1);
		quickSort(sData, temp + 1, n);
	}
}
int partition(double * sData, int m, int n){
	int i = m - 1;
	double x = sData[n];
	for (int j = m; j<n; j++){
		if (sData[j] <= x){
			i += 1;
			exchange(sData[i], sData[j]);
		}
	}
	exchange(sData[i + 1], sData[n]);
	return i + 1;
}
void siftdown(double* sData, int i, int n){
	int c = 2 * i + 1;
	if (c >= n)	return;
	if ((c + 1<n) && (sData[c + 1] > sData[c])) c++;
	if (sData[c] > sData[i]){
		exchange(sData[c], sData[i]);
		siftdown(sData, c, n);
	}
}
void HeapSort(double* sData, int n){
	for (int i = n - 1; i >= 0; i--)
		siftdown(sData, i, n);
	for (int i = n - 1; i >= 0; i--){
		exchange(sData[i], sData[0]);
		n--;
		for (int j = n - 1; j >= 0; j--)
			siftdown(sData, j, n);
	}
}
void exchange(double &a, double &b)
{
	double temp = a;
	a = b;
	b = temp;
}