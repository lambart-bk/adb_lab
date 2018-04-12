#include<iostream>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
using namespace std;
int main(int argc,char** argv)
{
	FILE *fp=fopen(argv[1],"r+");
	int i;
	while(fscanf(fp,"adsa %d\n",&i)!=EOF)
	{
		if(i%1024==atoi(argv[2]))
			cout<<"  "<<i<<endl;
	}
	fclose(fp);
}
