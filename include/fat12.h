#include <filesystem.h>

void FsFat12_Initialise();

FILE FsFat12_Open(const char* filename);

unsigned int FsFat12_Read(PFILE file, unsigned char* buffer, unsigned int length);

void FsFat12_Close(PFILE file);

pDirectoryEntry SearchDir(pDirectoryEntry p, const char* string, uint32_t fatNo);

uint16_t getFatEntry(uint32_t fatNo);

void getNextSearch(char* src, char* dest);

void strrem(char* src, char character);

void UpdateDirectory(char* string);

void ListDir();

void CurrDir();