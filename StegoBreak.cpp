/*
This program will ATTEMPT to extract hidden data from image files
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
//file signatures
const unsigned int JPEGHeader = 0xFFD8FF; //3 bytes
const unsigned int MP3 = 0x334449; // 3 bytes
const unsigned int BMPHeader = 0x4D42; //2 bytes
const unsigned int PDF = 0x46445025; //4 bytes
const unsigned long DOCX = 0x04034B50; // 4 bytes
const unsigned int maxHeadSize = 4;
//global variables for the CL argument image
unsigned int filesize = 0;
unsigned int end_location = 0; 
unsigned int row_size = 0;
unsigned int bits_per_pixel = 0;
signed int width = 0;

//convert bytes to unsigned number
unsigned int BytesToNumber(byte *b, int size) {
    unsigned int ret = 0;
    
    for (int i=0; i<size; i++) {
        ret = (b[i] << (i*8)) | ret;
    }

    return ret;
}
//function for reading bytes
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
//after reading the first row of the file, this will check to see if the first couple of bytes contain a file signature
unsigned int checkForValidHeader(byte* p, string &extension) {
	unsigned int holder = 0;

	holder = BytesToNumber(p, 3);
	if(holder == JPEGHeader) {
		extension = ".jpg";
		return 1;
	}
	if(holder == MP3) {
		extension = ".mp3";
		return 2;
	}
	holder = BytesToNumber(p, 2);
	if(holder == BMPHeader) {
		extension = ".bmp";
		return 3;
	}
	holder = BytesToNumber(p, 4);
	if(holder == PDF) {
		extension = ".pdf";
		return 4;
	}
	if(holder == DOCX) {
		extension = ".docx";
		return 5;
	}

	return 0; //if return 0, writeFile bool will be false and no file will be written

}
//function for single channel. works similiarly to double and triple channel
void doSingleChannel(ifstream &fin, string color, unsigned int readOffset) {
	
   signed int currReadLocation = 0; //holds address of current row to be read
   currReadLocation = filesize - row_size; //start at row 0 of bmp image
   unsigned int checkHeader = 0; //only used to check for a header after reading the first row
   bool writeFile = false; //will become true if a valid file signature is found in row 0
   unsigned int headerType = 0; // if a valid file sig is found, this will hold the kind of file sig to write later
   string OFileExtension = "_"; //this will be changed to the proper file sig if needed
   int eight_bits = 1; //incremented after a bit is added. when it is 8, p[i] is incremented to write to a new byte
   //memory for hidden files
   byte *p;
   p = (byte *)malloc(filesize/8);
   //index into memory for hidden files
   unsigned int countWriteLoc = 0;
   //will read up to the last row, <16000000 because of carry overflow for signed ints
   while(currReadLocation >= end_location + row_size && currReadLocation  < 16000000) {
   	if(checkHeader == 1) //check the first row for file signature
   	{
   		int holder;
   		holder = checkForValidHeader(p, OFileExtension);
   		if(holder != 0) {
   			writeFile = true;
   			headerType = holder;
   		}
   		else break;
   	}
   	//memory allocation for next row read
   	byte *b;
	b = (byte *)malloc(row_size); 
	//set location and read into memory
	fin.seekg(currReadLocation, fin.beg);
    fin.read((char *)b,row_size);
    //bit shift bytes read and append to the end of the hidden file being constructed
		for(int i = readOffset; i < row_size; i+=3) { //until read enough for one row
	    	b[i] = b[i] << 7;
	    	b[i] = b[i] >> 7;
	    	p[countWriteLoc] = (p[countWriteLoc] << 1)| b[i];
			if(eight_bits == 8) { //start a new byte if 8 bits have been written
				eight_bits = 0;
				countWriteLoc++;
				p[countWriteLoc] = 0;
			}
	    	eight_bits++;
		}
	//set up for next read	
	free(b);
	b = nullptr;
    checkHeader++;
    currReadLocation -= row_size;
   }

   //read final row of bmp file, same business as before
    byte *b;
    b = (byte *)malloc(row_size); 
	p[countWriteLoc] = 0;
	fin.seekg(end_location, fin.beg);
    fin.read((char *)b,row_size);
    eight_bits = 1; 
  	for(int i = readOffset; i < row_size; i+=3) { //until read enough for one row
    	b[i] = b[i] << 7;
    	b[i] = b[i] >> 7;
    	p[countWriteLoc] = (p[countWriteLoc] << 1)| b[i];
    	if(eight_bits == 8) {
    		eight_bits = 0;
    		countWriteLoc++;
    		p[countWriteLoc] = 0;
    	}
    	eight_bits++;
	}
	free(b);
	b = nullptr;


   //check for valid header before writing to a file
   if(writeFile == true) { 
   		ofstream Ofile;
   		Ofile.open(color + OFileExtension);
   		cout << "writing " << color << OFileExtension << endl;
   		Ofile.write((char *)p, filesize/16);
        Ofile.close();
        cout << endl;
   }
  

   free(p);
   p = nullptr;	
   
}
//same thing as single channel except for the bitshifting
void doDoubleChannel(ifstream &fin, string color, unsigned int firstreadOffset, unsigned int secondreadOffset) {
	
   signed int currReadLocation = 0; 
   currReadLocation = filesize - row_size; 
   unsigned int checkHeader = 0;
   bool writeFile = false;
   unsigned int headerType = 0;
   string OFileExtension = "_";
   
   byte *p;
   p = (byte *)malloc(filesize/4);
   unsigned int countWriteLoc = 0;
   
   while(currReadLocation >= end_location + row_size && currReadLocation  < 16000000) {
   	if(checkHeader == 1)
   	{
   		int holder;
   		holder = checkForValidHeader(p, OFileExtension);
   		if(holder != 0) {
   			writeFile = true;
   			headerType = holder;
   		}
   		else break;
   	}

   	byte *b;
	b = (byte *)malloc(row_size); 
	fin.seekg(currReadLocation, fin.beg);
    fin.read((char *)b,row_size);
   	
   	unsigned int increment = 1;
		for(int i = 0; i < row_size; i+=3) { //until read enough for one row
    	b[i+firstreadOffset] = b[i+firstreadOffset] << 7;
    	b[i+firstreadOffset] = b[i+firstreadOffset] >> 7;
    	p[countWriteLoc] = p[countWriteLoc] << 1 | b[i+firstreadOffset];
    	

    	b[i+secondreadOffset] = b[i+secondreadOffset] << 7;
    	b[i+secondreadOffset] = b[i+secondreadOffset] >> 7;
    	p[countWriteLoc] = (p[countWriteLoc] << 1)| b[i+secondreadOffset];

    	if(increment == 4) { //wrote 4 sets of 2 bits = 1 byte so increment index of p[]
    		countWriteLoc++;
    		increment = 1;
    	} else increment++;

	}
	free(b);
	b = nullptr;
    checkHeader++;
    currReadLocation -= row_size;
   }

   //read last row
    byte *b;
    b = (byte *)malloc(row_size); 
	fin.seekg(end_location, fin.beg);
    fin.read((char *)b,row_size);
  	unsigned int increment = 1;
		for(int i = 0; i < row_size; i+=3) { //until read enough for one row
    	b[i+firstreadOffset] = b[i+firstreadOffset] << 7;
    	b[i+firstreadOffset] = b[i+firstreadOffset] >> 7;
    	p[countWriteLoc] = p[countWriteLoc] << 1 | b[i+firstreadOffset];
    	
    	//extra set of bit shifting for 2 channels
    	b[i+secondreadOffset] = b[i+secondreadOffset] << 7;
    	b[i+secondreadOffset] = b[i+secondreadOffset] >> 7;
    	p[countWriteLoc] = (p[countWriteLoc] << 1)| b[i+secondreadOffset];

    	if(increment == 4) {
    		countWriteLoc++;
    		increment = 1;
    	} else increment++;

	}

	free(b);
	b = nullptr;


   //check for valid header before writing to a file
   if(writeFile == true) { 
   		ofstream Ofile;
   		Ofile.open(color + OFileExtension);
   		cout << "writing " << color << OFileExtension << endl;
   		Ofile.write((char *)p, filesize/4);
        Ofile.close();
        cout << endl;
   }
  

   free(p);
   p = nullptr;	
   
}
//same thing as above but with an extra set of bitshifting to account for 3rd channel
void doTripleChannel(ifstream &fin, string color, unsigned int firstreadOffset, int secondreadOffset, int thirdreadOffset) {
   signed int currReadLocation = 0; 
   currReadLocation = filesize - row_size; 
   unsigned int checkHeader = 0;
   bool writeFile = false;
   unsigned int headerType = 0;
   string OFileExtension = "_";
   
   byte *p;
   p = (byte *)malloc(filesize/4);
   unsigned int countWriteLoc = 0;
    int eight_bits = 1;
   while(currReadLocation >= end_location + row_size && currReadLocation  < 16000000) {
   	if(checkHeader == 1)
   	{
   		int holder;
   		holder = checkForValidHeader(p, OFileExtension);
   		if(holder != 0) {
   			writeFile = true;
   			headerType = holder;
   		}
   		else break;
   	}

   	byte *b;
	b = (byte *)malloc(row_size); 
	fin.seekg(currReadLocation, fin.beg);
    fin.read((char *)b,row_size);

   	unsigned int increment = 1;
		for(int i = 0; i < row_size; i+=3) { //until read enough for one row
    	b[i+firstreadOffset] = b[i+firstreadOffset] << 7;
    	b[i+firstreadOffset] = b[i+firstreadOffset] >> 7;
    	p[countWriteLoc] = p[countWriteLoc] << 1 | b[i+firstreadOffset];
    	increment++;
    	if(increment == 9) {
    		countWriteLoc++;
    		increment = 1;
    	} 
    	b[i+secondreadOffset] = b[i+secondreadOffset] << 7;
    	b[i+secondreadOffset] = b[i+secondreadOffset] >> 7;
    	p[countWriteLoc] = (p[countWriteLoc] << 1)| b[i+secondreadOffset];
    	increment++;

    	if(increment == 9) {
    		countWriteLoc++;
    		increment = 1;
    	} 
    	b[i+thirdreadOffset] = b[i+thirdreadOffset] << 7;
    	b[i+thirdreadOffset] = b[i+thirdreadOffset] >> 7;
    	p[countWriteLoc] = (p[countWriteLoc] << 1)| b[i+thirdreadOffset];
    	increment++;
    	if(increment == 9) {
    		countWriteLoc++;
    		increment = 1;
    	} 

	}
	free(b);

	b = nullptr;
    checkHeader++;
    currReadLocation -= row_size;
   }

   //read law row
    byte *b;
    b = (byte *)malloc(row_size); 
	fin.seekg(end_location, fin.beg);
    fin.read((char *)b,row_size);
  	unsigned int increment = 1;
	for(int i = 0; i < row_size; i+=3) { //until read enough for one row
    	b[i+firstreadOffset] = b[i+firstreadOffset] << 7;
    	b[i+firstreadOffset] = b[i+firstreadOffset] >> 7;
    	p[countWriteLoc] = p[countWriteLoc] << 1 | b[i+firstreadOffset];
    	

    	b[i+secondreadOffset] = b[i+secondreadOffset] << 7;
    	b[i+secondreadOffset] = b[i+secondreadOffset] >> 7;
    	p[countWriteLoc] = (p[countWriteLoc] << 1)| b[i+secondreadOffset];

    	if(increment == 3) {
    		countWriteLoc++;
    		increment = 0;
    	} 
    	b[i+thirdreadOffset] = b[i+thirdreadOffset] << 7;
    	b[i+thirdreadOffset] = b[i+thirdreadOffset] >> 7;
    	p[countWriteLoc] = (p[countWriteLoc] << 1)| b[i+thirdreadOffset];
    	increment++;

	}

	free(b);
	b = nullptr;


   //check for valid header before writing to a file
   if(writeFile == true) { 
   		ofstream Ofile;
   		Ofile.open(color + OFileExtension);
   		cout << "writing " << color << OFileExtension << endl;
   		Ofile.write((char *)p, filesize/4);
        Ofile.close();
        cout << endl;
   }
  

   free(p);
   p = nullptr;	
   
}
//set up global variables for image
void getImgInfo(ifstream &fin) {
    fin.seekg(0, fin.end);
    filesize = fin.tellg();
    fin.seekg(0, fin.beg);
    end_location = ReadBytes(fin, 0, 0x0A, 4); 

    fin.seekg(0, fin.beg);
    width = ReadBytes(fin, 0, 0x12, 2);
    fin.seekg(0, fin.beg);

    fin.seekg(0, fin.beg);
    bits_per_pixel = ReadBytes(fin, 0, 0x1C, 2);
    row_size = ((24 * width + 31)/ 32) * 4;

}

int main(int argc, char** argv) { 

	ifstream fin;

	if(argc !=2) { //ensure file was passed in CL
        std::cout << "FILE NOT FOUND. Attempting to open a default filename lena0.bmp\n";
        fin.open("lena0.bmp");
    } 
    else fin.open(argv[1]);

    if(fin.fail()) {
        cout << "Error, file could not open. Exiting program!\n";
       exit(1);
   }    
    //sets up variables for the reads
   getImgInfo(fin);
   //single channels
   doSingleChannel(fin, "Red", 0);
   doSingleChannel(fin, "Green", 1);
   doSingleChannel(fin, "Blue", 2);
   //double channels
   doDoubleChannel(fin, "RedGreen", 0, 1);
   doDoubleChannel(fin, "RedBlue", 0, 2);
   doDoubleChannel(fin, "GreenBlue", 1, 2);
   doDoubleChannel(fin, "GreenRed", 1, 0);
   doDoubleChannel(fin, "BlueRed", 2, 0);
   doDoubleChannel(fin, "BlueGreen", 2, 1);
   //triple channels
   doTripleChannel(fin, "RedGreenBlue", 0, 1, 2);
   doTripleChannel(fin, "RedBlueGreen", 0, 2, 1);
   doTripleChannel(fin, "GreenBlueRed", 1, 2, 0);
   doTripleChannel(fin, "GreenRedBlue", 1, 0, 2);
   doTripleChannel(fin, "BlueRedGreen", 2, 0, 1);
   doTripleChannel(fin, "BlueGreenRed", 2, 1, 0);
   //close
   fin.close();
   return 0;
}
