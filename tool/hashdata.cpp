#include<iostream>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
using namespace std;
#define HSize 50000
int main(int argc,char **argv)
{
	FILE *fp=fopen(argv[1],"r");
	int io,page_id;
	int hash[HSize];
	for(int i=0;i<HSize;i++)
	{
		hash[i]=-1;
	}
	long write=0;
	while(fscanf(fp,"%d,%d",&io,&page_id)!=EOF)
	{
		if(io==0)
		{
			if(page_id<1||page_id>HSize)
			{
				cout<<"page_id error "<<page_id<<endl;
				return 0;
			}
			hash[page_id-1]=0;
		}
		else if(io==1)
		{
			write++;
			if(page_id<1||page_id>HSize)
			{
				cout<<"page_id error"<<page_id<<endl;
				return 0;
			}
			hash[page_id-1]=1;
			//
		}	
		else
		{
			cout<<"error in if(io==) "<<io<<endl;
			return 0;
		}	
	}
	int a,b,c;
	a=b=c=0;
	for(int i=0;i<HSize;i++)
	{
		if(hash[i]==0)
			a++;
		else if(hash[i]==1)
			b++;
		else if(hash[i]==-1)
			c++;
		else
		{
			cout<<"error in hash value"<<hash[i]<<endl;
			return 0;
		}
	}
	cout<<"a:"<<a<<" b:"<<b<<" c:"<<c<<"---"<<a+b+c<<endl;
	cout<<"write time "<<write<<endl;
	fclose(fp);
	cout<<" end "<<endl;	
	return 0;
}
