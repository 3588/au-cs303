#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
using namespace std;
char* OpenFileNameChimp = "HEXA_Chimp.fasta";
char* OpenFileNameHs = "HEXA_HS.fasta";
char* OpenFileNameMouse = "HEXA_Mouse.fasta";
string ReadData(char*);
void run(string s1, string s2);
int GetMax(int t1, int t2, int t3);
int GetMaxFor4(int t1, int t2, int t3, int t4);
int main(){
	//read data from fil
	//the human chimp score is 3560. the human mouse is 790, an the chimp-mouse is 28.
	//3759(3750) , 556(472) , 170(95)
	string StrChimp = ReadData(OpenFileNameChimp);
	string StrHs = ReadData(OpenFileNameHs);
	string StrMouse = ReadData(OpenFileNameMouse);
	string t1 = "CGGATCGATCCTTCACCGCCCACCCGGTCGGGAGCTGCAGAATCCTTTGCTTACGGATCTCTGAGATCGAGCCCGCCTTGCTTCCCTCCCGTTCACGTGACCCTCCGATTGTCACGCGGGCGCCGCTCAGCTGACCGGGG";
	string t2 = "AGGGTGTGGGTCCTCCTGGGGTCGCAGGCGCAGAGCCGCCTCTGGTCACGTGATTCGCCGATAAGTCACGGGGGCGCCGCTCACCTGACCAGGGTCTCACGTGGCCAGCCCCCTCCGAGAGGGGAGACCAGCGGGCCATG";
	run(t1, t2);
	cout << "human chimp"<<endl;
	run(StrHs, StrChimp);
	return 0;
}
string ReadData(char* name){
	fstream file;
	string tempstr;
	ifstream in(name, ios::in);
	getline(in, tempstr);
	istreambuf_iterator<char> beg(in), end;
	string strdata(beg, end);
	strdata.erase(remove(strdata.begin(), strdata.end(), '\n'), strdata.end());
	//cout << strdata;
	in.close();
	return strdata;
}
void run(string s1, string s2){
	const int n = s1.length();
	const int m = s2.length();
	int Match = 2, MisMatch = -1 , Gap=-5;

	int **mat = new int*[n + 1];
	for (int j = 0; j < n + 1; j++) {
		mat[j] = new int[m + 1];
	}
	int minus2 = 0;
	for (int i = 0; i < m + 1; i++){
		mat[0][i] = minus2;
		minus2 = minus2 - 2;
	}
	minus2 = 0;
	for (int i = 0; i < n + 1; i++){
		mat[i][0] = minus2;
		minus2 = minus2 - 2;
	}
	for (int i = 1; i < n + 1; i++){
		for (int j = 1; j < m + 1; j++){
			int t1=0,t2=0,t3=0;
			if (s1[i-1] == s2[j-1]){
				t1 = mat[i - 1][j - 1];
				t1 = t1 + Match;
				//cout << "same: " << t1 << endl;
			}
			if (s1[i-1] != s2[j-1]){
				t1 = mat[i - 1][j - 1];
				t1 = t1 + MisMatch;
				//cout << "no same: " << t1 << endl;
			}
			t2 = mat[i][j - 1] + Gap;
			t3 = mat[i - 1][j] + Gap;
			mat[i][j] = GetMax(t1, t2, t3);
		}
	}
	cout<<"Best #: "<<mat[n][m]<<endl;

	//trace back
	string new_s1, new_s2;
	int dirction=0;//1 upper-lift, 2 lift, 3 up
	int total = 0, PositionN=n, PositionM=m;
	string GAP = "-";
	while (!PositionN == 0 && !PositionM==0){
		if (mat[PositionN][PositionM] == GetMaxFor4(mat[PositionN][PositionM], mat[PositionN - 1][PositionM - 1], mat[PositionN][PositionM - 1], mat[PositionN - 1][PositionM]) ||
			mat[PositionN - 1][PositionM - 1] == GetMaxFor4(mat[PositionN][PositionM], mat[PositionN - 1][PositionM - 1], mat[PositionN][PositionM - 1], mat[PositionN - 1][PositionM]))
			dirction = 1;
		else{
			if (mat[PositionN][PositionM - 1] == GetMaxFor4(mat[PositionN][PositionM], mat[PositionN - 1][PositionM - 1], mat[PositionN][PositionM - 1], mat[PositionN - 1][PositionM]))
				dirction = 2;
			if (mat[PositionN-1][PositionM] == GetMaxFor4(mat[PositionN][PositionM], mat[PositionN - 1][PositionM - 1], mat[PositionN][PositionM - 1], mat[PositionN - 1][PositionM]))
				dirction = 3;

		}
		
		//set value
		if (dirction == 1){
			new_s1 = new_s1+s1[PositionN-1];
			new_s2 = new_s2+s2[PositionM-1];
			total++;
			PositionN--;
			PositionM--;
		}
		if (dirction == 2){
			new_s1 = new_s1 + GAP;
			new_s2 = new_s2 + s2[PositionM-1];
			total++;
			PositionM--;
		}
		if (dirction == 3){
			new_s1 = new_s1 + s1[PositionN-1];
			new_s2 = new_s2+ GAP;
			total++;
			PositionN--;
		}
	}
	int Sp = total - 1, Ep = Sp - 70;
	while (Ep >=0){
		for (int i = Sp; i > Ep; i--){
			cout << new_s1[i];
		}
		cout << endl;
		for (int i = Sp; i > Ep; i--){
			if (new_s1[i] == new_s2[i])
				cout << "|";
			else
				cout << " ";
		}
		cout << endl;
		for (int i = Sp; i > Ep; i--){
			cout << new_s2[i];
		}
		cout << endl << endl;
		Sp = Ep;
		Ep = Sp - 70;
	}
}
int GetMax(int t1, int t2, int t3){
	if (t1>t2 && t1>t3) return t1; 
	if (t2>t1 && t2>t3) return t2;  
	return t3;
}
int GetMaxFor4(int t1, int t2, int t3, int t4){
	int a[] = { t1, t2, t3, t4 };
	int temp;
	for (int i = 0; i < 4;i++)
	for (int j = i + 1; j < 4;j++)
	if (a[i]>a[j]){
		temp = a[i];
		a[i] = a[j];
		a[j] = temp;
	}
	return a[3];
}