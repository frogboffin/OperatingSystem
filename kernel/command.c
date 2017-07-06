#include <command.h>
#include <console.h>
#include <keyboard.h>
#include <hal.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <floppydisk.h>
#include <fat12.h>

char currentCommand[0xff];
char argument1[0xff];
char prompt1[50] = "Command>";

void Run() 
{

	uint32_t inc = 0;
	uint32_t page = 0;
	char *cmd = currentCommand;
	char *arg1 = argument1;
	char *prompt = prompt1;
	
	ConsoleWriteString("\n");
	ConsoleWriteString(prompt);
	while (1)
	{
		keycode key = KeyboardGetCharacter();
		
		if (key == KEY_RETURN)
		{
/* 			ConsoleWriteCharacter(KeyboardConvertKeyToASCII(key));
			ConsoleWriteString("You wrote: ");
			ConsoleWriteString(cmd);
			ConsoleWriteString(" Your Argument: ");
			ConsoleWriteString(arg1);
			ConsoleWriteCharacter('\n');
			ConsoleWriteCharacter('\r'); */

			
			if (strcmp(cmd, "exit") == 0)
			{
				ConsoleWriteString("Operating System Shutting Down...");
				return;
			}
			if (strcmp(cmd, "prompt") == 0)
			{
				strcpy(prompt, arg1);
			}
			if (strcmp(cmd, "cls") == 0) ConsoleClearScreen(0x1F);
			
			if (strcmp(cmd, "read") == 0) Read(arg1);
			
			if (strcmp(cmd, "cd") == 0) CD(arg1);
			
			if (strcmp(cmd, "dir") == 0) ListDir();
			
			if (strcmp(cmd, "pwd") == 0) CurrDir();
			
			if (strcmp(cmd, "readdisk") == 0)
			{
				if (strcmp(arg1, "") == 0) arg1[0] = '0';
				int stepcount = 64;
				intptr_t *disk = (intptr_t*)FloppyDriveReadSector(arg1[0]-48);
				page = 0;
				
				while(((page+1) * stepcount) < 512)
				{
					if (KeyboardGetCharacter() == KEY_SPACE)
					{
						for (int i = 0; i < stepcount; i++)
						{
							ConsoleWriteInt(disk[(page * stepcount) + i],16);
							ConsoleWriteString(" ");
						}
						page++;
					}
				}
								
				ConsoleWriteCharacter('\n');
			}
			
			inc = 0;
			arg1 = 0;
			ConsoleWriteString(prompt);
		}
		else if (key == KEY_BACKSPACE)
		{

			ConsoleWriteCharacter(KeyboardConvertKeyToASCII(key));
			ConsoleWriteCharacter(' ');
			ConsoleWriteCharacter(KeyboardConvertKeyToASCII(key));
			inc--;
			currentCommand[inc] = '\0';
		}
		else if (KeyboardConvertKeyToASCII(key) != '\0')
		{
			if (key == KEY_SPACE)
			{
				inc++;
				arg1 = &currentCommand[inc];
			}
			else
			{
				(currentCommand[inc]) = KeyboardConvertKeyToASCII(key);
			
				(currentCommand[inc+1]) = '\0';
				
				inc++;
			}

			ConsoleWriteCharacter(KeyboardConvertKeyToASCII(key));
	
		}
	}
}

void Read(char* string)
{
	FILE file = FsFat12_Open(string);
	uint32_t b = 0;
	char buffer[1024];
	
	while(file.Eof == 0)
	{
		b += FsFat12_Read(&file, buffer, 50);
		
		ConsoleWriteString(buffer);		
	}
	
	ConsoleWriteString("\n Bytes Read: ");
	ConsoleWriteInt(b, 10);
	ConsoleWriteString("\n");
}

void CD(char* string)
{
	UpdateDirectory(string);
}
