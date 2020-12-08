/*
This program will analyze a memory dump file from a linux system and outputs the following information about the processes that were running:
PID and PPID, UID, total size of VMZ, memory addresses where descriptor starts, name of the process
*/

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <iomanip>
#include <fstream>
using namespace std;
typedef uint8_t byte;


//convert extracted bytes to unsigned long 
unsigned int BytesToNumber(byte *b, int size) {
    unsigned int ret = 0;
    
    for (int i=0; i<size; i++) {
        ret = (b[i] << (i*8)) | ret;
    }

    return ret;
}


//convert extracted bytes to signed long
long BytesToSignedNumber(byte *b, int size) {
    long ret = 0;
    
   for (int i=0; i<size; i++) {
        ret = (b[i] << (i*8)) | ret;
    }

    if (b[size-1] >= 0x80) { // if MSB is 1
        ret = ret | ((long(-1) >> (size*8)) << (size*8)); // sign extension
    }    
    
    return ret;
}

//convert bytes to string
string Ute16BytesToString(byte *b, unsigned int size) {
    string ret(size, ' '); //string of size size made with spaces
    
    for(int i =0; i < size; i++) {
    	if(b[i] == 0)
    		break;
    	ret[i] = char(b[i]);
    }
    

    return ret;
}

//reads in the file name and converts it to a properly formatted string
string readpName(ifstream &fin, unsigned int startingAddress, unsigned int size) {
	byte *b;
	b = (byte *)malloc(size);
	unsigned int offset = 0x194;
	startingAddress += offset;
    fin.seekg(startingAddress, fin.beg);
    fin.read((char *)b,size);
	string name;
	name = Ute16BytesToString(b, size);
	
	
	return name;
}

//function to read bytes
unsigned int ReadBytes(ifstream &fin, unsigned int origin, unsigned int offset, unsigned int size) {
    
	byte *b;
	b = (byte *)malloc(size);
    origin += offset;
    fin.seekg(origin, fin.beg);
    fin.read((char *)b,size);
    
    if (fin.gcount()!=size) {
        cout << "Unable to read " << size << " bytes at offset " << offset << ".\n";
        exit(1);
    }
    unsigned int ret;
    ret = BytesToNumber(b, size);
    return (unsigned int)ret;
}
//function to read bytes for a signed number
long SignedReadBytes(ifstream &fin, unsigned int origin, unsigned int offset, unsigned int size) {
    byte *b;
    b = (byte *)malloc(size);
    origin += offset;
    fin.seekg(origin, fin.beg);
    fin.read((char *)b,size);
    long ret;
   
    ret = BytesToSignedNumber(b, size);
    return ret;

 }

//used to find the address of the next PD 
 unsigned int findNextAddress(ifstream &fin, unsigned int Startingaddress, unsigned int size) {
	byte *b;
	b = (byte *)malloc(size);
	unsigned int offset = 0x7C;
    Startingaddress += offset;
    fin.seekg(Startingaddress, fin.beg);
       
    fin.read((char *)b,size);
    //cout << "reading from: " << Startingaddress << " \n";
    if (fin.gcount()!=size) {
        cout << "Unable to read " << size << " bytes at offset " << 0x7c << ".\n";
        exit(1);
    }
   unsigned int ret;
   ret = BytesToNumber(b, size);
   ret = (unsigned int)ret;
   return ret; //starting address of Process descriptor for next process


}

//prints a processes information
void print_pInfo(unsigned int PID, unsigned int PPID, unsigned int UID, unsigned int VMZ, unsigned int pd_start, string p_name) {
	cout << std::dec << PID << "\t\t" << std::dec << PPID << "\t\t" << std::dec << UID << "\t\t" << std::dec << VMZ << "\t\t" << std::hex << pd_start << "\t\t" << p_name << endl;

}


int main(int argc, char** argv) {
	if(argc !=2) { //ensure file was passed in CL
        std::cout << "FILE NOT FOUND\n";
    }
    ifstream fin;
	fin.open(argv[1]);
    if(fin.fail()) {
        cout << "Error, file could not open. Exiting program!\n";
        exit(2);
    } 
    cout  << "PID" << std::dec <<  "\t\tPPID" << std::dec << "\t\tUID" << std::dec << "\t\tVMZ" << std::hex << "\t\tTASK" << "\t\tCOMM" << endl;
     unsigned int start_add = 0x000660BC0;
     unsigned int  next_add = start_add;
     unsigned int check_next_add = 0;
    while(check_next_add != start_add) { //when the next address is the starting address, it's the end of the loop
    	//cout << "next add is: " << next_add << endl;
    	long PID = SignedReadBytes(fin, next_add, 0xA8, 4);
    	unsigned int PPID_loc = ReadBytes(fin, next_add, 0xB0, 4);
    	PPID_loc -= 0xC0000000;
    	unsigned int PPID = ReadBytes(fin, PPID_loc, 0xA8, 4);
    	unsigned int UID = ReadBytes(fin, next_add, 0x14C, 4);
    	unsigned int VMZ = 0; //holds sum of VMA
    	unsigned int MM_address = ReadBytes(fin, next_add, 0x84, 4);
    	//cout << "MM address is " << MM_address << endl;
    	bool VMA_end = true; //used to determine whether to crawl the VMA, also the end of the VMA and break out of while loop
    	unsigned int VMA_address = 0; //declare VMA address
    	if(MM_address != 0)
    		{
    			MM_address -= 0xC0000000;
    			//cout << "MM address is " << MM_address << endl;
    			VMA_address = ReadBytes(fin, MM_address, 0, 4); //mem address of first VMA 
    			if(VMA_address == 0)
    				{
    					 VMA_end = true; //if there's no VMA, then don't crawl the VMA (while loop is for false)
    					
    				}
    			else {
    				//cout << " read vMA address from " << MM_address << " and its " << VMA_address << endl;
    				VMA_address -= 0xC0000000;
    				 VMA_end = false; //it's not the end, so we crawl the VMA!
    				}
			}
    	 	
    	//cout << "VMA address b4 loop: " << VMA_address << endl;
    	//while loop for actually crawling the VMA
    	while(VMA_end == false) {
    		
    		unsigned int begin_VMA_add = ReadBytes(fin, VMA_address, 0x04, 4); 
    		unsigned int end_VMA_add = ReadBytes(fin, VMA_address, 0x08, 4);
    		//cout << "begin VMA " << begin_VMA_add << " and end VMA " << end_VMA_add  << endl;
    		unsigned int bytes_between = (end_VMA_add - begin_VMA_add) / 1024; //how much space to add to the total VMZ for the process, 1024 for KB
    		VMZ += bytes_between; //add the bytes to the running sum of the VMA
    		unsigned int next_VMA_add = ReadBytes(fin, VMA_address, 0x0C, 4); //potential next address for the next VMA
    		
    		if(next_VMA_add != 0) //it would be 0 if its the end
    		{
    			next_VMA_add -= 0xC0000000;
    			//cout << "next_VMA_add: " << next_VMA_add << endl;
    			VMA_address = next_VMA_add;

    		} else VMA_end = true; //set end to true if the next VMA address was 0

    	}

    	unsigned int TASK = next_add + 0xC0000000; //task is the current address plus the number to compensate for OS space
    	string p_name = readpName(fin, next_add, 16); //for COMM
    	print_pInfo(PID, PPID, UID, VMZ, TASK, p_name); //print process info
    	//cout << "current add is: " << next_add << " ";
    	unsigned int old_add = next_add; 
    	next_add = 0;
    	next_add = findNextAddress(fin, old_add, 4); //next process descriptor address
    	next_add -= 0xC0000000;	
    	next_add -= 0x7C; //beginning of process descriptor 
   	
    	check_next_add = next_add; //used for checking condition in while loop

    }



    return 0;
}