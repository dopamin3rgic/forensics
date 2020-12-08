/*
This program will extract partition information from a disk image following the GPT standard
*/
#include <stdlib.h>
#include <iostream>
using namespace std;
typedef unsigned char byte;

//function to print bytes in hex
void printInHex(byte *b, int size) {
	for(int i = 0; i < size; i++) {
		printf("%X", b[i]); //print each byte as hex
	}
}
//function to print bytes in dec
void printInDec(byte *b, int size) {
	unsigned long decimalRep;
	for(int i = 0; i < size; i++) {
		decimalRep = ((decimalRep << 8) | b[i] & 0xFFL);;
	}

	cout << int(decimalRep) << " ";
}
//function to convert to big endian
unsigned char* convertToBigE (byte *b, int size) {
	for(int i = 0; i < size/2; i++) { 
		byte tmp = b[i];
		b[i] = b[size-1-i];
		b[size-1-i] = tmp;
	}
	return b;
}
//reads in eight bytes until a GPT signature is found or end of file
int verifySignature(FILE *fin) {
	bool endOfLoop = false; //condition to exit loop when EOF is reached
	int numReads = 0;
	fseek(fin, 0, 0);
	unsigned char elements_read[8];
	unsigned char signature[8] = {0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54}; //GPT sig
	while(!endOfLoop){
		fread(elements_read, 1, 8, fin);
		++numReads; //counts how many times 8 bytes was read, used to calculate offset later in main
		for(int i = 0; i < 8; i++) {
			if(elements_read[i] == signature[i]) { 
				if(i == 7) { //all 7 bytes have to match to return the offset of the sig.
					return (numReads * 8) - 8; //num reads times the bytes in a read, minus 8 to get the offset of first byte
				} else continue; //if less than 7 elements match, keep checking for matches
			} else break; //if one of the bytes didn't match, start a new read
		}
		if(feof(fin))
			endOfLoop = true; //if end of file is reached, end the while loop
		
	}
	return 0; //if signature wasn't found, return 0 because offset was not found
}
//prints the LBA based off the offset from the signature given in main
void printLBA(FILE *fin, int offset, int offsetFromSig) {

	byte *b;
	int sizeofLBA = 8;
	b = (byte *)malloc(sizeofLBA);
	fseek(fin, offset + offsetFromSig, 0);
	fread(b, 1, sizeofLBA, fin);
	b = convertToBigE(b, sizeofLBA);
	printInDec(b, sizeofLBA);
	cout << " 0x";
	printInHex(b, sizeofLBA);
	cout << endl;
}
//function to print partition information in hex
int printPartitionInfo(FILE *fin, int offset, int offsetFromSig) {
	byte *b;
	int sizeofPart = 4;
	b = (byte *)malloc(sizeofPart);
	fseek(fin, offset + offsetFromSig, 0);
	fread(b, 1, sizeofPart, fin);
	b = convertToBigE(b, sizeofPart);
	printInDec(b, sizeofPart);
	cout << " 0x";
	printInHex(b, sizeofPart);
	cout << endl;
}
//print GUID in special format using multiple reads
void printPartitionGUID(FILE *fin, int offset) {
	byte *b;
	int firstRead = 4;
	int secondRead = 2;
	int thirdRead = 1;
	b = (byte *)malloc(firstRead);
	fseek(fin, offset, 0);
	fread(b, 1, firstRead, fin);
	b = convertToBigE(b, firstRead);
	printInHex(b, firstRead); //read 4 bytes , convert, print
	cout << "-";
	for(int i = 0; i < 2; i++) { //read 2 bytes twice , convert, print
		byte *c;
		c = (byte *)malloc(secondRead);
		fread(c, 1, secondRead, fin);
		c = convertToBigE(c, secondRead);
		printInHex(c, secondRead);	
		cout << "-";
	}
 
	for(int i = 0; i < 8; i++) { //read 1 byte eight times , convert, print
		byte *c;
		c = (byte *)malloc(thirdRead);
		fread(c, 1, thirdRead, fin);
		c = convertToBigE(c, thirdRead);
		if(i == 2)
			cout << "-";
		printInHex(c, thirdRead);	
	}
	cout << endl;
}
//print attribute flags for partitions in hex
void printAttributeFlag(FILE *fin, int offset, int offsetFromSig) {
	byte *b;
	int sizeofFlag = 8;
	b = (byte *)malloc(sizeofFlag);
	fseek(fin, offset + offsetFromSig, 0);
	fread(b, 1, sizeofFlag, fin);
	b = convertToBigE(b, sizeofFlag);
	cout << " 0x";
	printInHex(b, sizeofFlag);
	cout << endl;
}
//print the partition names using char()
void printPartitionName(FILE *fin, int offset, int offsetFromSig) {
	byte *b;
	int sizeofName = 72;
	b = (byte *)malloc(sizeofName);
	fseek(fin, offset + offsetFromSig, 0);
	fread(b, 1, sizeofName, fin);
	for(int i = 0; i < sizeofName; i++) {
		cout << char(b[i]);
	}
	cout << endl;
}
int main() {
	FILE *fin;
	fin = fopen("disk.img", "r");

	int byte_offset = verifySignature(fin);
	//cout << "byte offset to the signature is: " << byte_offset << endl; //debugging
	if(byte_offset == 0) {
		fclose(fin);
		cerr << "Signature not found. File closed and exiting program! \n";
		exit(2);
	}
	//print GUID information
	cout << "The Disk GUID is: ";
	printPartitionGUID(fin, byte_offset + 0x38);
	cout << "The first usable LBA for user partitions: " << endl;
	printLBA(fin, byte_offset, 0x28);
	cout << "\nThe last usable LBA for user partitions: "  << endl;
	printLBA(fin, byte_offset, 0x30);
	cout << "\nThe starting LBA where partition entries begin: " << endl;
	printLBA(fin, byte_offset, 0x48);
	cout << "\nNumber of Partition Entries: " << endl;
	printPartitionInfo(fin, byte_offset, 0x50);
	cout << "\nSize of a single partition entry: " << endl;
	printPartitionInfo(fin, byte_offset, 0x54);

	//variables for looping through the rest of the image
	int starting_partition = 0x400; //start for partition read at 0x400 and 
	int increment_partition = 128; // increment is partition size

	for(int i = 0; i < 128; i++) { //max i is 128 due to 128 possible partions
	byte *b;
	b = (byte *)malloc(16);
	fseek(fin, 0, starting_partition);
	fread(b, 1, 16, fin);
	if(b[i] == 0) //if the value of the candidate GUID is 0, it's probably not a GUID
		break; //stop looping for partition info
	else { //if it is a GUID, print all the things!
	cout << "\nInformation about partition " << i << ":\n";
	cout << "GUID: ";
	printPartitionGUID(fin, starting_partition);
	cout << "First LBA: ";
	printLBA(fin, starting_partition, 0x20);
	cout << "Last LBA: ";
	printLBA(fin, starting_partition, 0x28);
	cout << "Attribute Flag: ";
	printAttributeFlag(fin, starting_partition, 0x30);
	cout << "Partition Name: ";
	printPartitionName(fin, starting_partition, 0x38);
		}
	starting_partition += increment_partition; //increment to next candidate GUID address
	}

	fclose(fin); //close file and end program
	return 0;

}
