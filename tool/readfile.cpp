#include<iostream>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#define BUFFERSIZE 1024
#define MAXPAGES 50000
using namespace std;
int main(int argc,char** argv)
{
	FILE *fp=fopen(argv[1],"r+");
	char pagedata[BUFFERSIZE];
	int pagepointer;
	for( int i=0;i<MAXPAGES;i++){ //read basepage
	    fread(&pagepointer,sizeof(int),1,fp);
	    cout<<"base: "<<pagepointer<<endl;
	  }
	for( int i=0;i<MAXPAGES;i++){ //write datapage
	    fread(pagedata,sizeof(char),BUFFERSIZE,fp);
	    //cout<<"\npage data: "<<endl;
	    if(pagedata[1]=='n')
		cout<<"n: "<<i<<endl;
	    /*for(int j=0;j<BUFFERSIZE;j++)
		    cout<<" "<<pagedata[j]<<" ";*/
	  }  
	fclose(fp);
}
