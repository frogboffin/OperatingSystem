#include <fat12.h>
#include <console.h>
#include <bpb.h>
#include <floppydisk.h>
#include <filesystem.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

uint8_t fat[4608];
uint8_t *pfat = fat;
uint32_t fatStart;
uint32_t rootStart;
uint32_t dataStart;
uint32_t currDir;
char currDirName[13];
void FsFat12_Initialise()
{

	BootSector *boot = (BootSector*)FloppyDriveReadSector(0);
	BIOSParameterBlock bpb = boot->Bpb;
	
	fatStart = bpb.ReservedSectors;
	rootStart = fatStart + (bpb.SectorsPerFat * bpb.NumberOfFats);
	dataStart = rootStart + (bpb.NumDirEntries * sizeof(DirectoryEntry) / bpb.BytesPerSector);
	
	ConsoleWriteString("Fat Start: ");
	ConsoleWriteInt(fatStart, 10);
	ConsoleWriteCharacter('\n');

	ConsoleWriteString("Root Start: ");
	ConsoleWriteInt(rootStart, 10);
	ConsoleWriteCharacter('\n');

	ConsoleWriteString("Data Start: ");
	ConsoleWriteInt(dataStart, 10);
	ConsoleWriteCharacter('\n');

	currDir = rootStart;
	strcpy(currDirName, "\\");
	
	for (int i = 0; i < bpb.SectorsPerFat; i++)
	{
		memcpy(pfat + (bpb.BytesPerSector * i), FloppyDriveReadSector(fatStart+i), 512 * sizeof(uint8_t));
	}
	
/* 	for(int i = 0; i < 600; i++)
	{
		
		ConsoleWriteInt(fat[i], 16);
		ConsoleWriteString(" ");

	}
	ConsoleWriteCharacter('\n'); */

}

FILE FsFat12_Open(const char* filename)
{
	char fullSearchString[100]; //The full search string minus the last sub/file searched for
	strcpy(fullSearchString, filename);
	strrem(fullSearchString, '.');//remove fullstops from input
	
	//make input all uppercase
	for (int i = 0; i < strlen(fullSearchString); i++)
	{
		fullSearchString[i] = toupper(fullSearchString[i]);
	}
	pDirectoryEntry dir = (pDirectoryEntry)FloppyDriveReadSector(currDir);
	
/* 	ConsoleWriteString("\nSearching for (full string): ");			
	ConsoleWriteString(fullSearchString);
	ConsoleWriteCharacter('\n'); */
	
	pDirectoryEntry temp = SearchDir(dir, fullSearchString, dir->FirstCluster);
	
	FILE file;
	if (temp == 0) 
	{
		//ConsoleWriteString("\nNO FILE FOUND");
		//Return bad file
		file.Flags = FS_INVALID;
	}
	else
	{
		//ConsoleWriteString("\nFILE FOUND");
		//Return good file
		file.CurrentCluster = temp->FirstCluster;
		file.Position = 0;
		file.Eof = 0;
		file.FileLength = temp->FileSize;
		
		if((temp->Attrib & 0x10) == 0x10) 
			file.Flags = FS_DIRECTORY;
		else
			file.Flags = FS_FILE;
	}
	
	return file;
}

unsigned int FsFat12_Read(PFILE file, unsigned char* buffer, unsigned int length)
{
	if (file->FileLength == 0)
	{
		file->Eof = 1;
		return 0;
	}
	ConsoleWriteString("\n");
	buffer[0] = '\0';	
	uint32_t byteCount = 0;
		
	if(file->Flags == FS_INVALID) 
	{
		ConsoleWriteString("\nINVALID FILE");
		return 0;
	}
	
	if (length > file->FileLength) length = file->FileLength;
	
	uint8_t *data = FloppyDriveReadSector(file->CurrentCluster + dataStart - 2);
	for (int i = 0; i < length; i++)
	{
		if (file->Position >= file->FileLength)
		{
			FsFat12_Close(file);
			buffer[i] = '\0';
			return byteCount;
		}
		
		if(data[file->Position % 512] != '\0')
		{
			buffer[i] = data[file->Position % 512];
			byteCount++;
			file->Position++;
		}
 
		if (file->Position % 512 == 0)
		{
			file->CurrentCluster = getFatEntry(file->CurrentCluster);
			data = FloppyDriveReadSector(file->CurrentCluster + dataStart - 2);
		}
		

	}	

	return byteCount;
}

void FsFat12_Close(PFILE file)
{
	file->Eof = 1;
}

pDirectoryEntry SearchDir(pDirectoryEntry p, const char* string, uint32_t fatNo)
{
	char src[100];
	char dest[13];
	char dirName[13];

	strcpy(src, string);
	getNextSearch(src, dest);
	
	//ConsoleWriteString("\nSearching for ");
	//ConsoleWriteString(dest);
	
	pDirectoryEntry currDir = p;
	for (int i = 0; i < 16; i++)
	{
		memcpy(dirName, p->Filename, 12 * sizeof(char)); //Copy the full name/ext to the dirName string
		if((p->Attrib & 0x10) == 0x10) dirName[11] = ' ';
		dirName[12] = '\0'; //Add a null terminator
		strrem(dirName, ' '); //Remove all spaces
/* 		ConsoleWriteString("\nCurrent directory: ");
		ConsoleWriteString(dirName); */
		
		
		if(strcmp(dest, dirName) == 0)
		{
			if((p->Attrib & 0x10) == 0x10 && strcmp(src,dirName) != 0) //is a subdirectory?
			{
				//ConsoleWriteString("\nFound Folder");
				p = (pDirectoryEntry)FloppyDriveReadSector(p->FirstCluster + dataStart - 2);
				return SearchDir(p,src, p->FirstCluster);
			}
			else
			{
				//ConsoleWriteString("\n ===FOUND FILE: ===");
				return p;
			}
		}
		p++;
	}
	
	//if we get here then nothing in this sector matches our search
	//look to see if this is the last sector, if not then goto the next sector
	
	uint16_t fatEntry = getFatEntry(fatNo);
	
	if (fatEntry < 0xFF0 && fatEntry > 0x001)
	{
		p = (pDirectoryEntry)FloppyDriveReadSector(fatEntry + dataStart - 2);
		return SearchDir(p,string, p->FirstCluster); 
	}
	
	return 0;
}

uint16_t getFatEntry(uint32_t fatNo)
{
	uint32_t n = fatNo;
	uint16_t fatEntry;
	
	if (n % 2 == 0)
	{
		fatEntry = LO_NIBBLE(fat[(1+(3*n)/2)]);
		fatEntry <<= 8;
		fatEntry |= fat[(3*n)/2];
	}
	else
	{
		fatEntry = fat[1+(3*n)/2];
		fatEntry <<= 4;
		fatEntry |= HI_NIBBLE(fat[((3*n)/2)]);
	}
	
	return fatEntry;
}

void getNextSearch(char* src, char* dest) 
{
	char tempDest[13];
	char *pTempDest = tempDest;
	char tempSrc[100];
	strcpy(tempSrc, src);
	char *pTempSrc = tempSrc;

	while(*pTempSrc != '\\' && *pTempSrc != '/' && *pTempSrc != 0)
	{
		*(pTempDest++) = *(pTempSrc++);
	}
	*pTempDest = '\0';
	strrem(pTempDest, ' ');
	*pTempSrc = '\0';
	
	if (strlen(++pTempSrc) != 0) strcpy(src, pTempSrc); //increased to cut the /
	if (strlen(tempDest) != 0) strcpy(dest, tempDest);
}

void strrem(char* src, char character)
{
	char tempSrc[100];
	strcpy(tempSrc, src);
	char *pTempSrc = tempSrc;

	char tempDest[100];
	char *pTempDest = tempDest;
	
	while(*pTempSrc != '\0')
	{
		if (*pTempSrc != character) 
			*pTempDest++ = *pTempSrc;
		
		*pTempSrc++;
	}
	*pTempDest = '\0';

	strcpy(src, tempDest);
}

void UpdateDirectory(char* string)
{
	for (int i = 0; i < strlen(string); i++)
	{
		string[i] = toupper(string[i]);
	}
	
	pDirectoryEntry dir = (pDirectoryEntry)FloppyDriveReadSector(currDir);
	dir = SearchDir(dir,string, dir->FirstCluster);
	
	if((dir->Attrib & 0x10) == 0x10) string[11] = ' ';
	string[12] = '\0'; //Add a null terminator
	strrem(string, ' '); //Remove all spaces
	strcpy(currDirName, string);
	currDir = dir->FirstCluster + dataStart - 2;
	ConsoleWriteString("\n");
}

void ListDir()
{
	char dirName[13];
	pDirectoryEntry p;
	uint16_t fatEntry = 0;
	
	ConsoleWriteString("\n");
	p = (pDirectoryEntry)FloppyDriveReadSector(currDir);
	fatEntry = (currDir);
	
	while(fatEntry < 0xFF0 && fatEntry > 0x001)
	{
		for (int i = 0; i < 16; i++)
		{
			memcpy(dirName, p->Filename, 12 * sizeof(char)); //Copy the full name/ext to the dirName string
			if((p->Attrib & 0x10) == 0x10) dirName[11] = ' ';
			dirName[12] = '\0'; //Add a null terminator
			strrem(dirName, ' '); //Remove all spaces
			if(strcmp(dirName, "") != 0)
			{
				ConsoleWriteString(dirName);
				ConsoleWriteString("\n");
			}
			p++;
		}
		p = (pDirectoryEntry)FloppyDriveReadSector(fatEntry + dataStart - 2);
		fatEntry = getFatEntry(p->FirstCluster);

	}
}

void CurrDir()
{
	ConsoleWriteString("\nCurrent Directory: ");
	ConsoleWriteString(currDirName);
	ConsoleWriteString("\n");
}