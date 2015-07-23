#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
                                                                                
using namespace std;
   char s[100] ;
   char t[100] ;
   int a[100][100] ;
   char align_s[100] ;
   char align_t[100];
   int len = 0 ,match = 0 , mis = 0;
                                                                                
                                                                                
int p( int i , int j)
 {
    if(s[i]==t[j]) return match;
    else return mis;
 }
int max (int x ,int y , int z)
 {
     int temp ;
     if(x>=y) temp =x ;
     else temp = y ;
     if(temp >=z) return temp;
     else return z;
 }
int min_1 (int x , int y , int z)
{
    int temp ;
    if(x > y) temp =y ;
    else temp = x ;
    if(temp > z) return z;
    else return temp;
}
void align (int i , int j )
 {
     if( i==0 && j==0 )
     {
         len = 0 ;
     }
else if (i > 0 && a[i][j] == a[i-1][j]+mis )
          {
               align ( i-1 , j );
               align_s[len] = s[i-1];
               align_t[len] = '-';
               len=len+1 ;
          }
     else if ( i > 0  &&  j > 0 && a[i][j]== a[i-1][j-1]+ p(i-1 , j-1 ) )
         {
              align (i-1 , j-1 );
              align_s[len] = s[i-1];
              align_t[len] = t[j-1];
              len = len +1 ;
         }
     else if ( j > 0  && a[i][j]==a[i][j-1] +mis)
         {
               align (i , j-1 );
               align_s[len] = '-';
               align_t[len] = t[j-1];
               len =len +1 ;
         }
                                                                                
 }
int main(int argc, char *argv[])
{
                                                                                
   int g ;
   printf("input s =>");
   scanf("%s",s);
   printf("input t =>");
   scanf("%s",t);
   printf("match =>");
   scanf("%d",&match);
   printf("mismatch =>");
   scanf("%d",&mis);
   printf (" \n%d %d\n" ,strlen(s),strlen(t));
   for(int i = 0 ; i < strlen(s)+1 ; i++)
    {
      a[i][0] = i*(mis);
    }
   for(int j = 0 ; j < strlen(t)+1 ; j++)
    {
      a[0][j] = j*(mis);
    }
   for( int i = 1 ;  i <strlen(s)+1 ; i++)
    {
for (int j =1 ; j <strlen(t)+1 ; j++)
      {
       if(match < mis)
      {
       a[i][j] = min_1(a[i-1][j]+mis , a[i-1][j-1] + p(i-1 , j-1) , a[i][j-1]+mis);
      }
       else if (match > mis)
       {
        a[i][j] = max(a[i-1][j]+mis , a[i-1][j-1] + p(i-1 , j-1) , a[i][j-1]+mis);
       }
        else
        { printf("權重一樣\n"); break;}
                                                                                
      }
    }
                                                                                
     for( int i = 0 ; i <strlen(s)+1 ; i++)
    {
     for (int j = 0 ; j <strlen(t)+1 ; j++)
      {
        printf("%3d" ,a[i][j] );
      }
      printf("\n");
 }
                                                                                
  align(strlen(s) , strlen(t) );
  cout << align_s << " \n" ;
  cout << align_t << " \n";
  system("PAUSE");
                                                                                
}

