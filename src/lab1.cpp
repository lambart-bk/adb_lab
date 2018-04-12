#include<iostream>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include <boost/concept_check.hpp>
#include <chrono>
#define FRAMESIZE 4096
#define BUFFERSIZE 1024
#define MAXPAGES 50000
using namespace std;

typedef struct {
	char field[FRAMESIZE];
	int size=FRAMESIZE;
}bFrame;

typedef struct {
	int frame_id;
	int page_id;
}NewPages;

typedef struct struct_BCB{
	int page_id;
	int frame_id;
	int latch;
	int count;
	int dirty_bit;
	struct_BCB *next; 
}BCB;

typedef struct struct_lru{	
  int frame_id;
  struct struct_lru *pre;
  struct struct_lru *next;
	
}LRU_node;

//data storage manager
class DSMgr
{
	public:
		DSMgr();
		~DSMgr();
		int openFile(string filename);
		int closeFile();
		bFrame readPage(int page_id);
		int writePage(int page_id,bFrame frm);
		int seek(int offset,int pos);
		FILE *getFile();
		void incNumPage();
		int getNumPage();
		void setUse(int index,int use_bit);
		int getUse(int index);
		int foundEmptyPage();
		
	private:
		FILE *currFile,*filePointer;
		int numPage;
		int pages[MAXPAGES]; //page use bit,1:use 0:can reusable -1:NULL 
		int page_size;  
		int pos_data_start; //data block start position in file 
};
DSMgr::DSMgr() {
	currFile = NULL;
	filePointer=NULL;
	numPage = 0;
	pos_data_start=MAXPAGES*sizeof(int );
	page_size=FRAMESIZE;
	openFile("./data.dbf");
	for(int i=0;i<MAXPAGES;i++){
	  if(readPage(i).size!=-1)
	    pages[i]=1;
	  else
	    pages[i]=0;
	}
}
DSMgr::~DSMgr() {
	closeFile();
}
int DSMgr::openFile(string filename){
	currFile=fopen(filename.c_str(),"r+");
	if(currFile!=NULL)
	  return 1;
	else{
	  cout<<"openfile failed!"<<endl;
	  return 0;
	}
}
int DSMgr::closeFile(){
	fclose(currFile);	
}
bFrame DSMgr::readPage(int page_id){
	bFrame temp;
	char ch[FRAMESIZE];
	int offset;
	seek(page_id*sizeof(int),0);
	if(fread(&offset,sizeof(int),1,filePointer)==0){
	  cout<<"r-fread(&offset,"<<page_id<<"*sizeof(int),1,filePointer) error"<<endl;
	  exit(0);
	}
	seek(offset,pos_data_start);
	if(fread(ch,sizeof(char),FRAMESIZE,filePointer)==0)
	{
	  //cout<<"fread "<<page_id<<" error "<<endl;
	  temp.size=-1;
	  return temp;
	}
	for(int i=0;i<FRAMESIZE;i++)
	  temp.field[i]=ch[i];
	return temp;
}
int DSMgr::writePage(int page_id,bFrame frm){
	char ch[FRAMESIZE];
	for(int i=0;i<FRAMESIZE;i++)
	  ch[i]=frm.field[i];
	int offset;
	seek(page_id*sizeof(int),0);
	if(fread(&offset,sizeof(int),1,filePointer)==0){
	  cout<<"w-fread(&offset,"<<page_id<<"*sizeof(int),1,filePointer) error"<<endl;
	  exit(0);
	}
	seek(offset,pos_data_start);
	int result=fwrite(ch,sizeof(char),FRAMESIZE,filePointer);
	if(result!=0){
	  incNumPage();
	  setUse(page_id,1);
	}
	return result;
}
int DSMgr::seek(int offset ,int pos){
	filePointer=currFile;
	if(fseek(filePointer,pos,0)!=0){
	  cout<<"seek pos error !"<<endl;
	  return -1;
	}
	if(fseek(filePointer,offset,1)!=0){
	  cout<<"seek offset error !"<<endl;
	  return -2;	  
	}
	return 0;
}
FILE* DSMgr::getFile(){
	return currFile;
}
void DSMgr::incNumPage(){
	numPage++;
}
int DSMgr::getNumPage(){
	return numPage;
}	
void DSMgr::setUse(int index,int use_bit){
	if(use_bit==1 || use_bit==0)
		pages[index]=use_bit;
	else
	{
		cout<<"use_bit error: "<<index<<"-"<<use_bit<<endl;
		exit(0);
	}
}
int DSMgr::getUse(int index){
	return pages[index];
}
int DSMgr::foundEmptyPage(){
  for(int i=0;i<MAXPAGES;i++){
      if(getUse(i)==0)
	return i;
  }
  cout<<"disk have not empty page"<<endl;;
  return -1;
}

//buffer manager
class BMgr
{
	public:
		BMgr();
		~BMgr();
		//Interface fuction
		bFrame readPage(int page_id);
		int writePage(int page_id,bFrame frm);
		int fixPage(int page_id,int prot);
		NewPages fixNewPage();
		void unfixPage(int page_id);
		int getNumFreePage();
		
		//Internal fuction
		int  selectVictim();
		int  Hash(int page_id);
		void removeBCB(BCB *ptr,int page_id);
		void deleteLRUele(int frid);
		void insertLRUele(int frid);
		int  popLRUele();
		void setDirty(int frame_id);
		void unsetDirty(int frame_id);
		void WriteDirty();
		void WriteDirty(BCB* bcb);
		void PrintFrame(int frame_id);
		long count_io;
		int getLru_size();
	private:
		DSMgr dsmgr;
		//buffer 
		bFrame *Buf;
		//bFrame Buf[BUFFERSIZE];
		int  buf_free[BUFFERSIZE]; //1:use ,0:free
		int numFreeFrame;
		LRU_node * lru_head;
		LRU_node * lru_tail;
		int lru_size,lru_maxsize;
		//hash table
		int ftop[BUFFERSIZE];
		BCB* *ptob;
		//BCB* ptob[BUFFERSIZE];
};
BMgr::BMgr(){
	Buf=(bFrame*)malloc(BUFFERSIZE*sizeof(bFrame));
	ptob=(BCB**)malloc(BUFFERSIZE*sizeof(BCB*));
	count_io=0;
	for(int i=0;i<BUFFERSIZE;i++)
	  buf_free[i]=0;
	numFreeFrame=BUFFERSIZE-1;
	lru_head=lru_tail=NULL;
	lru_maxsize=BUFFERSIZE;
	lru_size=0;
}
BMgr::~BMgr(){
	WriteDirty();
}

bFrame BMgr::readPage(int page_id){
	return Buf[fixPage(page_id,1)];
}

int BMgr::writePage(int page_id,bFrame frm){
	//int frame_id=fixPage(page_id,1);

	int flag=Hash(page_id);
	if(flag!=-1){	//in buffer,modify
	  //update lru_linklist
	  deleteLRUele(flag);
	  insertLRUele(flag);
	  Buf[flag]=frm;
	  setDirty(flag);//dirty data
	  return 1;	
	}

	//not in buffer ,write into buffer and disk
	int victim=selectVictim();
	//to buffer 
	Buf[victim]=frm;
	ftop[victim]=page_id; //change ftop
	BCB *block=(BCB*)malloc(sizeof(BCB)); //change ptob
	//new block 
	block->count=0;
	block->latch=0;
	block->dirty_bit=0;
	block->frame_id=victim; 
	block->page_id=page_id;
	block->next=NULL;
	BCB* bcb_head=ptob[page_id%BUFFERSIZE],*last;
	if(bcb_head==NULL){	
	  bcb_head=block;
	  ptob[page_id%BUFFERSIZE]=bcb_head;
	  //cout<<"create block first"<<page_id<<endl; //debug output
	}
	else{  
	  while(bcb_head!=NULL){
	    last=bcb_head;		
	    bcb_head=bcb_head->next;
	  }
	  last->next=block;
	  //cout<<"create block  "<<page_id<<endl;//debug output  //debug output
	}	
	//into disk
	//DSMgr dsmgr;
	dsmgr.writePage(page_id,frm); 
	count_io++;
	
	return 2;



	/*int flag=Hash(page_id);
	if(flag!=-1){ //in buffer
	  Buf[flag]=frm;
	  //set dirty dirty_bit
	  setDirty(flag);
	  WriteDirty();
	}
	else{	//not in buffer 
	  //fixNewPage();
	  
	}*/ 
}

int BMgr::fixPage(int page_id,int prot){
	int flag=Hash(page_id);
	if(flag!=-1){	//in buffer,return frame_id
	  //update lru_linklist
	  deleteLRUele(flag);
	  insertLRUele(flag);
	  //cout<<"fixPage end(in buffer)"<<endl;
	  return flag;
	}
	//not in buffer ,load from datastorage
	//DSMgr dsmgr;
	bFrame temp;
	temp=dsmgr.readPage(page_id); 
	count_io++;
	int victim=selectVictim();
	//replace 
	Buf[victim]=temp;
	ftop[victim]=page_id; //change ftop
	BCB *block=(BCB*)malloc(sizeof(BCB)); //change ptob
	//new block 
	block->count=0;
	block->latch=0;
	block->dirty_bit=0;
	block->frame_id=victim; 
	block->page_id=page_id;
	block->next=NULL;
	BCB* bcb_head=ptob[page_id%BUFFERSIZE],*last;
	if(bcb_head==NULL){	
	  bcb_head=block;
	  ptob[page_id%BUFFERSIZE]=bcb_head;
	  //cout<<"create block first"<<page_id<<endl; //debug output
	}
	else{  
	  while(bcb_head!=NULL){
	    last=bcb_head;		
	    bcb_head=bcb_head->next;
	  }
	  last->next=block;
	  //cout<<"create block  "<<page_id<<endl;//debug output  //debug output
	}	
/*cout<<""<<endl;
cout<<page_id%BUFFERSIZE<<"-- "<<page_id<<endl;;
BCB * p=ptob[page_id%BUFFERSIZE];
while(p!=NULL){
  cout<<p->page_id<<" ";
  p=p->next;
}	
cout<<endl<<""<<endl;*/
	//cout<<"fixPage end(not in buffer)"<<endl;
	return victim;
}
void BMgr::unfixPage(int page_id){
	int flag=Hash(page_id);
	if(flag!=-1){
	  buf_free[flag]=0; //be free 
	  lru_size--;
	  deleteLRUele(flag); //delete from lru_linklist
	  //TODO:delete page in disk ?
	}
	else{
	  cout<<"unfixPage("<<page_id<<")---fuction error: "<<endl;
	  exit(0);
	}
  
}
NewPages BMgr::fixNewPage(){
	//DSMgr dsmgr;
	NewPages temp;
	temp.page_id=dsmgr.foundEmptyPage();
	count_io++;
	temp.frame_id=selectVictim();
	return temp;
}
int BMgr::selectVictim(){
	if(lru_size<lru_maxsize){  //if have free buffer ,malloc 
	  //insert into lru  
	  int frid;
	  for(int i=0;i<BUFFERSIZE;i++){
	    if(buf_free[i]==0){
	      frid=i;
	      buf_free[i]=1;
	      break;
	    }
	  }
	  //insert into lru_linklist 
	  insertLRUele(frid);
	  lru_size++;
	  return frid;
	}
	else{
	  //select from lru ,pop and insert into lru_linklist,removeBCB
	  int frid=popLRUele();
	  int page_id=ftop[frid];	  
	  insertLRUele(frid);	  
	  /*cout<<""<<endl;
	  cout<<page_id%BUFFERSIZE<<"-- "<<page_id<<endl;;
	  BCB * p=ptob[page_id%BUFFERSIZE];
	  while(p!=NULL){
	    cout<<p->page_id<<" ";
	    p=p->next;
	  }	
	  cout<<endl<<""<<endl;*/
	  removeBCB(ptob[page_id%BUFFERSIZE],page_id);
	  /*if(page_id==40143){
	  cout<<""<<endl;;
	  BCB * p=ptob[page_id%BUFFERSIZE];
	  while(p!=NULL){
	    cout<<p->page_id<<" ";
	    p=p->next;
	  }
	  cout<<""<<endl;;
	}*/
	    return frid;
	}
}
void BMgr::deleteLRUele(int frid)  //delete frid/node from linklist
{
	LRU_node *p=lru_head;
	while(p!=NULL&&p->frame_id!=frid){
	  p=p->next;
	}
	if(p==NULL){
	  cout<<frid<<" not in lru_linklist"<<endl;
	  exit(0);
	}
	else if(p==lru_head){
	  p->next->pre=NULL;
	  lru_head=p->next;
	  free(p);
	}
	else if(p==lru_tail){
	  p->pre->next=NULL;
	  lru_tail=p->pre;
	  free(p);
	}
	else{
	  p->pre->next=p->next;
	  p->next->pre=p->pre;
	  free(p);
	}
	
}
void BMgr::insertLRUele(int frid) //insert into tail of linklist
{
	LRU_node* temp=(LRU_node*)malloc(sizeof(LRU_node));
	temp->frame_id=frid;
	if(lru_tail==NULL){
	  temp->next=NULL;
	  temp->pre=NULL;
	  lru_tail=temp;
	  lru_head=temp;
	}
	else{
	  lru_tail->next=temp;
	  temp->pre=lru_tail;
	  temp->next=NULL;
	  lru_tail=temp;
	}
}

int BMgr::popLRUele(){ //pop frid as victim
	if(lru_head==NULL){
	  cout<<"lru_head is NULL"<<endl;
	  exit(0);
	}
	int victim=lru_head->frame_id;
	LRU_node *p=lru_head;
	p->next->pre=NULL;
	lru_head=p->next;
	free(p);
	return victim;
}
void BMgr::removeBCB(BCB *ptr,int page_id){   //this is only called by selectVictim()
	//cout<<"removeBCB called--"<<page_id<<endl; //debug output
	BCB* last,*current;	
	last=current=ptr; 
	while(current!=NULL&&current->page_id!=page_id)
	{
	  last=current;
	  current=current->next;
	}
	if(current==NULL)
	{
	  //cout<<"error: "<<page_id<<" is not in BCB "<<endl; //debug output
	  //cout<<"ptr->page_id: "<<ptr->page_id<<endl;;	//debug output
	  exit(0);
	}
	else if(current==ptr) //found in head
	{
	  ptr=current->next;
	  ptob[page_id%BUFFERSIZE]=ptr;
	  if(current->dirty_bit==1)
	    WriteDirty(current);
	  free(current);
	} 
	else
	{
	  last->next=current->next;
	  if(current->dirty_bit==1)
	    WriteDirty(current);
	  free(current);
	}
	/*if(page_id==40143){   //for debug
	  cout<<""<<endl;;
	  BCB * p=ptr;
	  while(p!=NULL){
	    cout<<p->page_id<<" ";
	    p=p->next;
	  }
	  cout<<""<<endl;;
	}*/
}
int BMgr::getNumFreePage(){
	return numFreeFrame;
}
void BMgr::unsetDirty(int frame_id){
	BCB* temp;
	temp=ptob[(ftop[frame_id])%BUFFERSIZE];
	while(temp!=NULL){
	  if(temp->frame_id ==frame_id)
	    temp->dirty_bit=0;
	  else
	    temp=temp->next;
	}
	//cout<<"error:frame_id- "<<frame_id<<" is not in buffer/bcb "<<endl;//debug output
}
void BMgr::setDirty(int frame_id){
	BCB* temp;
	temp=ptob[(ftop[frame_id])%BUFFERSIZE];
	while(temp!=NULL){
	  if(temp->frame_id ==frame_id){
	    temp->dirty_bit=1;				
	    //cout<<"setDirty end"<<endl;
	    return ;
	  }
	  else
	    temp=temp->next;
	}
	//cout<<"error:frame_id- "<<frame_id<<" is not in buffer/bcb "<<endl;//debug output
}
void BMgr::WriteDirty(){
	BCB* temp;
	for(int b_id=0;b_id<BUFFERSIZE;b_id++)
	{
	    temp=ptob[(ftop[b_id])%BUFFERSIZE];
	    while(temp!=NULL){
	      if(temp->dirty_bit==1)
	      {
		WriteDirty(temp);
	      }
	      temp=temp->next;
	    }
	}
	cout<<"WriteDirty end"<<endl;
}
void BMgr::WriteDirty(BCB* bcb){
	//DSMgr dsmgr;
	dsmgr.writePage(bcb->page_id,Buf[bcb->frame_id]);
	count_io++;
	//cout<<"WriteDirty(bcb) end"<<endl;
}
int BMgr::Hash(int page_id){
	BCB* temp=ptob[page_id%BUFFERSIZE];  //bcb* head for page_id
	while(temp!=NULL){
	  //cout<<"hash--page_id: "<<temp->page_id<<endl;	//debug output
	  //cout<<"hash--temp->frame_id:"<<temp->frame_id<<endl;  //debug output
	  if(temp->page_id==page_id)
	    return temp->frame_id;
	  else
	    temp=temp->next;
	}
	//cout<<"page_id- "<<page_id<<" is not in buffer/bcb "<<endl;//debug output
	return -1; //not found
}
void BMgr::PrintFrame(int frame_id){
	cout<<"Buf["<<frame_id<<"]: "<<endl;
	for(int i=0;i<BUFFERSIZE;i++)
		cout<<Buf[frame_id].field[i];
	cout<<endl;
}
int BMgr::getLru_size(){
  return lru_size;
}
void printdata(bFrame bf){
  cout<<"data: "<<endl;
  for(int i=0;i<bf.size;i++)
    cout<<bf.field[i];
  cout<<endl;
}
void materialized(){
  FILE *fp=fopen("./data.dbf","w+");
  if(fp==NULL){
    cout<<"openFile failed"<<endl;
    exit(0);
  }
  int offset;
  char pagedata[FRAMESIZE];
  for(int i=0;i<FRAMESIZE;i++)
    pagedata[i]='*';
  for( int i=0;i<MAXPAGES;i++){ //write basepage
    offset=i*sizeof(pagedata); 
    fwrite(&offset,sizeof(int),1,fp);
  }
  for( int i=0;i<MAXPAGES;i++){ //write datapage
    fwrite(pagedata,sizeof(char),FRAMESIZE,fp);
  }  
  fclose(fp);
}

int main(int argc,char** argv){
  materialized();  
  BMgr bmgr;
  /*bFrame temp;
  temp.size=FRAMESIZE;
  for(int i=0;i<FRAMESIZE;i++)
    temp.field[i]='/';
  DSMgr dsmgr;
  printdata(dsmgr.readPage(25645));cout<<endl<<endl;
  dsmgr.writePage(25645,temp);
  printdata(dsmgr.readPage(25645));cout<<endl<<endl;*/
    
  //TODO:check BUFFERSIZE and FRAMESIZE ！！！
  /*bFrame writeFrame;  ////debug set
  bmgr.writePage(29243,writeFrame);cout<<endl<<endl;
  bmgr.writePage(2619,writeFrame);cout<<endl<<endl;
  bmgr.writePage(9787,writeFrame);cout<<endl<<endl;
  bmgr.writePage(8763,writeFrame);cout<<endl<<endl;
  bmgr.Hash(571);*/
  

  FILE *fp=fopen("./data-5w-50w-zipf.txt","r+");
  if(fp==NULL){
    cout<<"file not found !"<<endl;
    return -1;
  }
  int io,page_id;
  bFrame writeFrame,readFrame;
  for(int i=0;i<writeFrame.size;i++)
    writeFrame.field[i]='n';
  long i=0;	//debug output
  chrono::steady_clock::time_point t1 = chrono::steady_clock::now();
  while(fscanf(fp,"%d,%d",&io,&page_id)!=EOF){
    //cout<<io<<","<<page_id<<endl; //debug output
    if(i++%100==0)  //debug output
    	cout<<i<<endl;  //debug output
    if(io==1){ //write
      bmgr.writePage(page_id,writeFrame);
    }
    else if(io==0){	//read
      readFrame=bmgr.readPage(page_id);
    }
    else
      cout<<"read error: data-5w-50w-zipf.txt"<<endl;
    if(i==atoi(argv[1])) //debug for small scale test
      break;
  }
  cout<<"io : "<<bmgr.count_io<<endl;
  fclose(fp);
  chrono::steady_clock::time_point t2 = chrono::steady_clock::now();
  chrono::duration<double> time_used = chrono::duration_cast<chrono::duration<double>>(t2-t1);
  cout<<"costs time: "<<time_used.count()<<" seconds."<<endl;
}
