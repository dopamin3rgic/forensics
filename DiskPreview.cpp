/*
This program will extract partition information from a NTFS partition disk image
leftover comments are written for explanations and/or debugging
*/
#include <stdlib.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <algorithm>
using namespace std;
typedef uint8_t byte;


// function to print bytes in dec
void printInDec(byte *b, int size) {
	unsigned long decimalRep;
	for(int i = 0; i < size; i++) {
		decimalRep = ((decimalRep << 8) | b[i] & 0xFFL);;
	}

	cout << int(decimalRep);
}
// function to convert to big endian
byte* convertToBigE (byte *b, int size) {
	for(int i = 0; i < size/2; i++) { 
		byte tmp = b[i];
		b[i] = b[size-1-i];
		b[size-1-i] = tmp;
	}
	return b;
}

//convert extracted bytes to unsigned long 
unsigned long BytesToNumber(byte *b, int size) {
    unsigned long ret = 0;
    
    for (int i=0; i<size; i++) {
        ret = (b[i] << (i*8)) | ret;
    }

    return ret;
}

//convert extracted bytes to signed long
long BytesToSignedNumber(byte *b, int size) {
    unsigned long ret = 0;
    
    for (int i=0; i<size; i++) {
        ret = (b[i] << (i*8)) | ret;
    }

    if (b[size-1] >= 0x80) { // if MSB is 1
        ret = ret | ((long(-1) >> (size*8)) << (size*8)); // sign extension
    }    
    return ret;
}

//function to read in bytes from file and return a unsigned long number for use in print functions and calculations
unsigned long ReadBytes(FILE *fin, unsigned long origin, unsigned long offset, int size) {
    byte *b;
    b = (byte *)malloc(size);
    fseek(fin, offset, origin);
    fread(b, size, 1, fin);

    unsigned long ret;
   
    ret = BytesToNumber(b, size);

    return ret;
}

// used to find the first attribute of a MFT record
unsigned long FindfirstAttribute(FILE *fin, unsigned long record_start) {
	//cout << "reading first attribute from record start: " << record_start << endl;
	unsigned long locationfirstAttribute;
	locationfirstAttribute = record_start + 0x14; //offset where location of first attribute is 20 is 0x14
	//cout << "location first attribute: " << locationfirstAttribute << endl;
	unsigned long hold_value_from_read;
	hold_value_from_read = ReadBytes(fin, 0, locationfirstAttribute, 1);
	unsigned long attribute_ID_location;
	attribute_ID_location = record_start + hold_value_from_read; //read the offset of the starting location into a value
	//unsigned long locationOfsizeofAttribute = attribute_ID_location + 0x4; // location of size is 4 bytes from attribute ID
	//b = convertToBigE(b, 1);

	//attribute_ID_location = BytesToNumber(b, 1); //offset from record file to the first ID location
	//cout << "offset of attribute start" << attribute_ID_location << endl;
	//attribute_ID_location += 0x4; //offset to size
	

	//cout << "location of the first attribute is: " << attribute_ID_location << endl;

	return attribute_ID_location;

}

// find the beginning of the MFT by reading boot sector. returns the starting address of the MFT
unsigned long findMFTLocation(FILE *fin, int offset) {
	unsigned long MFT_Address;
	byte *b;
	int sizeOfMFTRead = 8;
	b = (byte *)malloc(sizeOfMFTRead);
	fseek(fin, offset, 0);
	fread(b, 1, sizeOfMFTRead, fin);
	MFT_Address = BytesToNumber(b, sizeOfMFTRead);
	b = convertToBigE(b, sizeOfMFTRead);
	return MFT_Address;

}

//used to find the address of the next attribute within a record
unsigned long findNextAddress(FILE *fin, unsigned long Startingaddress) {
	byte *b;
	b = (byte *)malloc(4);
	Startingaddress += 0x04;
	fseek(fin, Startingaddress, 0);
	fread(b, 1, 4, fin);
	unsigned long ret;
	ret = BytesToNumber(b, 4);
	//cout << "size of file is " << ret << endl;
	return ret;


}

//used to read in the value of an attribute flag, returns it as unsigned long
unsigned long checkAttributeFlag(FILE *fin, unsigned long startingAddress) {
	byte *b;
	b = (byte *)malloc(1);
	
	fseek(fin, startingAddress, 0);
	fread(b, 1, 1, fin);
	unsigned long ret;
	ret = BytesToNumber(b, 1);
	//cout << "attribute flag is " << ret << endl;
	return ret;
} 

//read lower byte in a data run
unsigned long read_data_attribute_lower(FILE *fin, unsigned long startingAddress) {
	unsigned long start_of_data_run = startingAddress + 0x40;
	byte *b;
	b = (byte *)malloc(1);
	fseek(fin, start_of_data_run, 0);
	fread(b, 1, 1, fin);
	unsigned long lower_byte;
	
	lower_byte = BytesToNumber(b,1);
	lower_byte = (lower_byte & 0xF);

	return lower_byte;
	
}

//read upper byte in a data run
unsigned long read_data_attribute_upper(FILE *fin, unsigned long startingAddress) {
	unsigned long start_of_data_run = startingAddress + 0x40;
	byte *b;
	b = (byte *)malloc(1);
	fseek(fin, start_of_data_run, 0);
	fread(b, 1, 1, fin);
	unsigned long upper_byte;
	upper_byte = BytesToNumber(b, 1);
	upper_byte = upper_byte >> 4;
	return upper_byte;
	
}

//calculate value of the second component of DR
unsigned long read_data_run_second(FILE *fin, unsigned long startingAddress, unsigned long bsecondcomp) {
	byte *b;
	b = (byte *)malloc(bsecondcomp);
	fseek(fin, startingAddress, 0);
	fread(b, bsecondcomp, 1, fin);
	unsigned long secondcomp;
	secondcomp = BytesToNumber(b, bsecondcomp);
	//cout << "second component is: " << secondcomp;
	return secondcomp;

}

//calculate value of the third component of DR
unsigned long read_data_run_third(FILE *fin, unsigned long startingAddress, unsigned long bthirdcomp) {
	byte *b;
	b = (byte *)malloc(bthirdcomp);
	fseek(fin, startingAddress, 0);
	fread(b, bthirdcomp, 1, fin);
	unsigned long thirdcomp;
	thirdcomp = BytesToNumber(b, bthirdcomp);
	//cout << " third component is: " <<  thirdcomp << endl;
	return thirdcomp;
}

//print functions for MFT records
void print_later_records(string name, unsigned long CN, unsigned long FBCN, unsigned long offset, int num_alt_streams) {
	cout << name << ": " << endl;
	cout << "\t" << CN/512 << " :: " << FBCN;
	
	cout << "(+" << offset << ") ";
	if(num_alt_streams != 0) {
		cout << ":: " << num_alt_streams << " ";
	}
}

//used to convert bytes read from a file read to a string
string Ute16BytesToString(byte *b, int size) {
    string ret(size/2,' ');
    int v;
    
    for (int i=0; i<size; i+=2) {
        v = (b[i+1] << 8) | b[i];
        ret[i/2] = (char)v;
    }
    

    return ret;
}

//reads in the file name and converts it to a properly formatted string
string readFileName(FILE *fin, unsigned long startingAddress, unsigned long size) {
	//startingAddress += 0x5A;
	//cout << "reading file name starting at address: " << startingAddress << endl;
	byte *b;
	b = (byte *)malloc(size);
	fseek(fin, startingAddress, 0);
	fread(b, size, 1, fin);
	string name;
	name = Ute16BytesToString(b, size);
	string name_without_spaces;
	for(int i = 0; i < name.size(); i++) {
		if(name[i] != '0')
			name_without_spaces += name[i];
	}
	return name_without_spaces;
}

/*bool checkforResident(FILE *fin, unsigned long startingAddress) {
	unsigned long value_of_res_flag_read = 0x00;
	value_of_res_flag_read = ReadBytes(fin, startingAddress, 0x08, 1);
	if(value_of_res_flag_read == 0x00) { 
		cout << "candidate is a resident! " << endl;
		return true;
	}
		
	if(value_of_res_flag_read == 0x01) {
		cout << "candidate is nonresident" << endl;
		return false;
	}
	cout << "Error for resident flag! read in " << value_of_res_flag_read << " at offset 0x08 from address: " << startingAddress << endl;
	return false;

}*/

int main(int argc, char** argv) {
	if(argc !=2) { //ensure file was passed in CL
        std::cout << "FILE NOT FOUND";
    }
    FILE *fin;
	fin = fopen(argv[1], "r");
    if(fin==NULL) {
        cout << "Error, file could not open. Exiting program!\n";
        exit(2);
    } 

    

	   unsigned long MFT_Address = 0;
	   MFT_Address = findMFTLocation(fin, 0x30);
	   //long something = 0;
	   //something = findSomething(fin, 0x40);
	   //cout << "\n" <<  MFT_Address << " " << something << endl;
	  MFT_Address *= 512; //multiple by size of cluster to get address
	  unsigned long size_of_drive = 502000000;
	  unsigned long size_of_MFT = (size_of_drive * .125) / 512; //size of the MFT table
	  unsigned long end_of_MFT = MFT_Address + size_of_MFT; //address to stop parsing for new records using calculated size
	  //cout << "drive ends at: " << end_of_MFT << endl;
	  unsigned long current_start_of_record = MFT_Address; //start reading records at the beginning of MFT 
	  //record one
	  //int count_num_records = 0;
	  //unsigned long bytes_read = 0;
	  while(current_start_of_record < end_of_MFT) { //read until end of MFT table
	  //cout << "current start of record: " << current_start_of_record / 512 << "and end of MFT: " << end_of_MFT << endl;
		  unsigned long max_record_length = current_start_of_record + 0x400; //every record at most 1024 bytes
		  unsigned long firstAttributeAddress;
		  firstAttributeAddress = FindfirstAttribute(fin, current_start_of_record); //address of first attribute
		  unsigned long size_of_first; 
		  size_of_first = findNextAddress(fin, firstAttributeAddress); //size of first attribute read in using a function
		  unsigned long loop_address = firstAttributeAddress + size_of_first; //starting address of the 2nd attribute in the first record
		  unsigned long data_attribute_address = 0;
		  unsigned long file_attribute_address = 0;
		  
		  bool valid_data_attribute = false; //only want to print out records with valid data/file attributes so we must keep track of whether this record has them
		  bool valid_file_attribute = false;
		  bool check_for_extra_streams = false; //used to check for extra data streams
		  int count_potential_data_streams = 0;  //used to count extra data streams if check_For_data_streams is true
		  for(int i = 2; i < 8; i++) { //forloop is mostly just a way to iterate through potential attributes, the value of i doesn't really matter because it will break when we reach the end of a record anyway
		  	if(data_attribute_address) {
		  		check_for_extra_streams = true; //if a data attribute was already found, then check for extra streams 
		  		
		  	}
		  	unsigned long Attribute_flag; //holds the current value of the attribute flag
		  	Attribute_flag = checkAttributeFlag(fin, loop_address);
		  	if(Attribute_flag == 0x30) { //if the attribute flag is for a file, save the address and set valid_file_attribute to true for later use
		  		//cout << " I found the filename attribute at address: " << loop_address << "!\n";
		  		file_attribute_address = loop_address; //address where 0x30 is 
		  		valid_file_attribute = true;
		  	}

		  	if(Attribute_flag == 0x80) { //if the attribute flag is for a data, save the address and set valid_data_attribute to true for later use
		  		//cout << " I found the data attribute at address: " << loop_address << "!\n";
		  		if(!check_for_extra_streams)
		  			data_attribute_address = loop_address; //only set data attribute address if it's the first one, not an alt
		  		valid_data_attribute = true;
		  		if(check_for_extra_streams) {
		  			count_potential_data_streams++; //if this isn't the first 0x080 attribute, count the alt data streams
		  		}

		  	}
		  	//fseek(fin, 0, 0);
		  	unsigned long size_of_loop_att;
		  	size_of_loop_att = findNextAddress(fin, loop_address); //use to find next attribute to loop through
		  	if(loop_address + size_of_loop_att >= max_record_length)  { //if you reached the end of a record, break out of forloop that loops through attributes
		  		break;
		  	}
		  	
		  	loop_address += size_of_loop_att; //increment address to check next attribute within record
		  	//cout << "loop address: " << loop_address << endl;

		  } //end of forloop for attributes
			if(!valid_data_attribute || !valid_file_attribute ) { //after reaching the end of the record, if there isn't valid data/file attributes then increment address and read next record
				 current_start_of_record += 0x400;
				continue;
			}	
			
			string filename; //holds the filename for the record
			
			if(valid_file_attribute) {
				
				unsigned long read_file_size;
				read_file_size = findNextAddress(fin, file_attribute_address);
				unsigned long filename_size;
				filename_size = read_file_size - 0x5A; //how many bytes to read in for filename. size of the file attribute minus the offset the filename starts at
				//cout << "file attribute address: " << file_attribute_address << " and file attribute size: " << read_file_size << endl;
				
				filename = readFileName(fin, file_attribute_address + 0x5A, filename_size); //store filename
				

			}	
		  
			unsigned long LCN;
			
		    if(valid_data_attribute) {	
		    	//bool isResident = false;
		  		//isResident = checkforResident(fin, data_attribute_address);
		  		
			  	unsigned long data_run_second_component = read_data_attribute_lower(fin, data_attribute_address); //reads in upper and lower bytes of the data run
			  	unsigned long data_run_third_component = read_data_attribute_upper(fin, data_attribute_address);
			  	//cout << "lower byte is: " << data_run_second_component << " and upper byte is: " << data_run_third_component << endl;
			  
			  	unsigned long count;
			  	count = read_data_run_second(fin, data_attribute_address+0x41, data_run_second_component);
			  	unsigned long offset_for_dr;
			    offset_for_dr = data_attribute_address + 0x41 + data_run_second_component; //calculate offset using second component
			  	LCN = read_data_run_third(fin, offset_for_dr, data_run_third_component); //read in LCN for data run
			  	 
			  	
			  	 if(LCN == 0 && count == 0 && filename != "$Repair") { //resident
			  		unsigned long offset;
			  		offset = (data_attribute_address + 0x18) % current_start_of_record;
			  		print_later_records(filename, current_start_of_record, current_start_of_record/512, offset, count_potential_data_streams);
			  		cout << endl;
			  	}
			  	else { //nonresident
			  		print_later_records(filename, current_start_of_record, LCN, 0, count_potential_data_streams);
			  		if(filename != "$Repair")
			  			cout << ":: (" << LCN << "," << count << ")";
		  			cout << endl;
			  		}
			 
			 	}



		   current_start_of_record += 0x400; //increment address to read next record
	  }//end of while loop




    return 0;

}