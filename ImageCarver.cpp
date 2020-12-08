/*
This program will ATTEMPT to extract JPEG images from a file representing the unallocated space found on a disk
*/

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <iomanip>
using namespace std;
typedef uint8_t byte;
const unsigned int JPEGHeader = 0xFFD8FF;
const unsigned int JPEGHeader2 = 0xD8FF;
const unsigned int JPEGFooter = 0xD9FF;
 
//convert extracted bytes to unsigned int 
unsigned int BytesToNumber(byte *b, int size) {
    unsigned int ret = 0;
    
    for (int i=0; i<size; i++) {
        ret = (b[i] << (i*8)) | ret;
    }

    return ret;
}

unsigned int ReadBytes(ifstream &fin, unsigned int origin, unsigned int offset, unsigned int size) {
    
	  byte *b;
	  b = (byte *)malloc(size);
    origin += offset;
    fin.seekg(origin, fin.beg);
    fin.read((char *)b,size);
    
    if (fin.gcount()!=size) {
        unsigned int unreadable_loc = fin.tellg();
        cout << "Unable to read " << size << " bytes at offset " << unreadable_loc << ".\n";
        exit(1);
    }
    unsigned int ret;
    ret = BytesToNumber(b, size);
    return (unsigned int)ret;
}

int main(int argc, char** argv) {
	
	ifstream fin;

	if(argc !=2) { //ensure file was passed in CL
        std::cout << "FILE NOT FOUND. Attempting to open a default filename unalloc.img\n";
        fin.open("unalloc.img");
    } 
    else fin.open(argv[1]);

    if(fin.fail()) {
        cout << "Error, file could not open. Exiting program!\n";
       exit(1);
   }

   // get file size
   fin.seekg(0, fin.end);
   unsigned int filesize = fin.tellg();
   fin.seekg(0, fin.beg);
  
   unsigned int currRead = 0; //used to hold whatever is being currently read in 
   unsigned int currReadLocation = 0; //use to keep of where a file should begin looking for another header. if found holds header location
   unsigned short count_headers = 0; //use for naming pictures
   cout << "Filename" << setw(15) << "Starting Loc" << setw(15) << "Ending loc" << setw(10) << "Size" << endl;

   while(currReadLocation < filesize-2) {
   	unsigned int increment_value = 1; //default read for header
   	currRead = ReadBytes(fin, currReadLocation, 0, 3);
   	//crawl file until a JPEG header is found, increment by 1 byte every time

   	if(currRead == JPEGHeader) {

   		count_headers++;
   		bool find_footer = false; //breaks loop if another footer was found
		  bool another_header = false;   //breaks loop if another footer was foun
		  unsigned int fragFileEnd = 0;     //if another header was found, it'll save the potential ending location of the fragmented
   		unsigned int newReadLoc = currReadLocation + 3; //newReadLoc will eventually hold the location of a footer, but here it is a starting point for crawling

   		while(find_footer == false && another_header == false) { //loop until you find a header or footer
   			currRead = ReadBytes(fin, newReadLoc, 0, 2);
   			if(currRead == JPEGFooter)
   			{
   				find_footer = true;

   			}

   			if(currRead == JPEGHeader2)
   			{
   				fragFileEnd = newReadLoc - 1;
   				another_header = true;

   			}

  					   			
   			newReadLoc += 0x01;
   		}
   		//make output file of picture data
   		if(another_header) { //fragmented file case
   			ofstream picfile;
	   		string picname = "pic" + to_string(count_headers) + ".jpg";
	   		picfile.open(picname);
	   		//prepare to write
	   		unsigned int numBytesToRead = fragFileEnd - currReadLocation;
	   		byte *p;
			  p = (byte *)malloc(numBytesToRead);
			  //read picture information
			  fin.seekg(currReadLocation, fin.beg);
	    	fin.read((char *)p,numBytesToRead);
	    	//write to file
	    	picfile.write((char *)p, numBytesToRead);
	    	//close and deallocate space
	    	picfile.close();
	    	free (p);
	    	p = nullptr;
	    	cout << picname << setw(10) << "0x" << std::hex << currReadLocation << setw(10) << "0x" << std::hex << fragFileEnd << std::dec << setw(10) << numBytesToRead << endl;  
   			currReadLocation = fragFileEnd + 1; //set start of the next read to the end of the last image

   		}
   		else { //normal picture case
	   		ofstream picfile;
	   		string picname = "pic" + to_string(count_headers) + ".jpg";
	   		picfile.open(picname);
	   		//prepare to write
	   		unsigned int numBytesToRead = newReadLoc - currReadLocation + 1;
	   		byte *p;
			  p = (byte *)malloc(numBytesToRead);
			  //read picture information
			  fin.seekg(currReadLocation, fin.beg);
	    	fin.read((char *)p,numBytesToRead);
	    	//write to file
	    	picfile.write((char *)p, numBytesToRead);
	    	//close and deallocate space
	    	picfile.close();
	    	free (p);
	    	p = nullptr;
	    	cout << picname << setw(10) << "0x" << std::hex << currReadLocation << setw(10) << "0x" << std::hex << newReadLoc+2 << std::dec << setw(10) << numBytesToRead << endl;  
   			currReadLocation = newReadLoc; //set start of the next read to the end of the last image
    	}
   		
   	}
   	else {
   		currReadLocation += increment_value; //if no header was found, add 1 and try reading again
   		
   	}

   }

   fin.close();
   return 0;
}

