/*
Junjun Huang
i. Row 1: d for directed and u for undirected
ii. Row 2: a for adjacency matrix and l for linked list
iii. Row 3: the number of nodes (must be >0)
iv. The linked list or adjacency matrix
*/
#include<iostream>
#include<string> 
#include <vector>
#include <fstream>
#include <strstream>
#include <list>
using namespace std;
/* From internet https://github.com/AlloyTeam/webtop/blob/master/myutil.cpp*/
string& replace_all(string& str, const string& old_value, const string& new_value)
{
	while (true)
	{
		int pos = 0;
		if ((pos = str.find(old_value, 0)) != string::npos)
			str.replace(pos, old_value.length(), new_value);
		else break;
	}
	return str;
}
/* From internet https://github.com/AlloyTeam/webtop/blob/master/myutil.cpp*/

class List {
	 public:
	   int no;
	   string node;
};
class Graph{
	//for BFS and DFS
	int V; 
	list<int> *adj;
	//for BFS and DFS
	vector<List> vec;
	int TotalNumberNodes;
	char Row1, Row2;
public:
	Graph(){
		TotalNumberNodes = NULL;
		Row1 = NULL;
		Row2 = NULL;
		vec.clear();
	}
	~Graph(){}
	Graph & operator= (const Graph & other){
		vec = other.vec;
	
	};
	bool operator==(const Graph &other)  {
		vector<List> Tvec = other.vec;
		if (vec.size() == Tvec.size())
		{
			vector<List>::iterator iter1 = vec.begin();
			vector<List>::iterator iter2 = Tvec.begin();
			for (; iter1 != vec.end(); ++iter1)
			if (iter1->no == iter2->no&&iter1->node == iter2->node)
				return true;
			else
				return false;
		}
		else
			return false;
			
	};
	bool operator!=(const Graph &other)  {
		vector<List> Tvec = other.vec;
		if (vec.size() == Tvec.size())
		{
			vector<List>::iterator iter1 = vec.begin();
			vector<List>::iterator iter2 = Tvec.begin();
			for (; iter1 != vec.end(); ++iter1)
			if (iter1->no == iter2->no&&iter1->node == iter2->node)
				return false;
			else
				return true;
		}
		else
			return true;
	};
	void InputData(char* name);
	void OutputData();
	bool EdgeBtNodes(int a, int b);
	void DeleteNode(int n);
	void DeleteEdge(int a, int b);
	void BFS(int s);
	void DFSTool(int v, bool checked[]);
	void DFS(int s);
	

};
void Graph::InputData(char* name){
	string tempstr;
	ifstream in(name, ios::in);
	getline(in, tempstr);
	Row1 = tempstr[0];
	getline(in, tempstr);
	Row2 = tempstr[0];
	getline(in, tempstr);
	TotalNumberNodes = tempstr[0] - 48;
	if (Row2 == 'l'){
	for (int i = 0; i < TotalNumberNodes; i++)
	{
		List list;
		list.no = i;
		getline(in, list.node);
		list.node = replace_all(list.node, "#", "");
		vec.push_back(list);
	}}
	if(Row2 == 'a'){
		   string Ts;
		for (int i = 0; i<TotalNumberNodes; i++)
		{
			List list;
			list.no = i;
			getline(in, Ts);
			for (int j = 0; j < Ts.size();j++)
			if (Ts[j] == '1')
			{
				strstream Tss;
				string TTs;
				Tss << j+1;
				Tss >> TTs;
				list.node += TTs;
			}
				vec.push_back(list);
				cout << endl;
		}
	}
}
void Graph::OutputData(){
	vector<List>::iterator iter = vec.begin();
	for (; iter != vec.end(); ++iter)
		cout << "node. = " << iter->no+1 << ", link node = " << iter->node << endl;
}
bool Graph::EdgeBtNodes(int a, int b){
	if (a > TotalNumberNodes || b > TotalNumberNodes) return false;
	else{
		vector<List>::iterator iter = vec.begin();
		for (; iter != vec.end(); ++iter)
		{
			if (iter->no==a-1)
			for (int i = 0; i < iter->node.size(); i++)
			if (iter->node[i] - 48 == b)
				return true;
		}
		return false;
	}
}
void Graph::DeleteNode(int n){
	if (n > TotalNumberNodes) cout << "No Nodes";
	else
	{
		vector<List> Tvec;
		List list;
		strstream Tss;
		string Ts;
		Tss << n;
		Tss >> Ts;
		vector<List>::iterator iter = vec.begin();
		for (; iter != vec.end(); ++iter)
		{
			if (iter->no != n - 1)
			{
				list.no = iter->no;
				list.node = replace_all(iter->node, Ts, "");
				Tvec.push_back(list);
			}
		}
		vec = Tvec;
	}
}
void Graph::DeleteEdge(int a, int b){
	if (EdgeBtNodes(a, b))
	{
		vector<List> Tvec;
		List list;
		strstream Tssa,Tssb;
		string Ta, Tb;
		Tssa << a;
		Tssa >> Ta;
		Tssb << b;
		Tssb >> Tb;
		vector<List>::iterator iter = vec.begin();
		for (; iter != vec.end(); ++iter)
		{
			
			list.no = iter->no;
			list.node = iter->node;
			if (Row1 == 'd'){
				if (list.no == a - 1){
					list.node = replace_all(list.node, Tb, "");
				}
			}
			if (Row1 == 'u'){
				if (list.no == a - 1)
					list.node = replace_all(list.node, Tb, "");
				if (list.no == b - 1)
					list.node = replace_all(list.node, Ta, "");
			}
			Tvec.push_back(list);
		}
		vec = Tvec;
	}
}
void Graph::BFS(int s){
	V = TotalNumberNodes+1;
	adj = new list<int>[V];
	vector<List>::iterator iter = vec.begin();
	for (; iter != vec.end(); ++iter){
		for (int i = 0; i < iter->node.length(); i++){
			adj[iter->no + 1].push_back(iter->node[i] - 48);
		}
	}

	bool *checked = new bool[V];
	for (int i = 1; i < V+1; i++)
		checked[i] = false;

	list<int> queue;
	checked[s] = true;
	queue.push_back(s);
	list<int>::iterator itera;
	cout << "Breadth First Traversal : ";
	while (!queue.empty())
	{
		s = queue.front();
		cout << s << " ";
		queue.pop_front();

		for (itera = adj[s].begin(); itera != adj[s].end(); ++itera)
		{
			if (!checked[*itera])
			{
				checked[*itera] = true;
				queue.push_back(*itera);
			}
		}
	}
	}
void Graph::DFSTool(int v, bool checked[])
{

	checked[v] = true;
	cout << v << " ";
	list<int>::iterator itera;
	for (itera = adj[v].begin(); itera != adj[v].end(); ++itera)
	if (!checked[*itera])
		DFSTool(*itera, checked);
}
void Graph::DFS(int s)
{
	V = TotalNumberNodes + 1;
	adj = new list<int>[V];
	vector<List>::iterator iter = vec.begin();
	for (; iter != vec.end(); ++iter){
		for (int i = 0; i < iter->node.length(); i++){
			adj[iter->no + 1].push_back(iter->node[i] - 48);
		}
	}
	bool *checked = new bool[V];
	cout << "Depth First Traversal : ";
	for (int i = 1; i < V+1; i++)
		checked[i] = false;
	DFSTool(s, checked);
}
int main()
{

	Graph test1;
	cout << "Test for LinkList\n";
	test1.InputData("1.txt");
	test1.OutputData();
	//cout << "is edge between node 1 and 4\n";
	//cout<<test1.EdgeBtNodes(1, 4)<<endl;
	//cout << "Delete node 1\n";
	//test1.DeleteNode(1);
	//test1.OutputData();
	//cout << "Delete edge 1 and 3\n";
	//test1.DeleteEdge(1,3);
	//test1.OutputData();
	test1.BFS(1);
	test1.DFS(2);
	Graph test2;
	cout << "\n\nTest for Matrix\n";
	test2.InputData("3.txt");
	test2.OutputData();
	//cout << "is edge between node 1 and 4\n";
	//cout << test2.EdgeBtNodes(1, 4) << endl;
	//cout << "Delete node 1\n";
	//test1.DeleteNode(1);
	//test1.OutputData();
	//cout << "Delete edge 1 and 3\n";
	//test2.DeleteEdge(1, 3);
	//test2.OutputData();
	test2.BFS(1);
	test2.DFS(3);
	return 0;
}