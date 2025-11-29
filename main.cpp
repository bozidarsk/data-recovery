#include <iostream>
#include <fstream>

#ifdef linux
#include <unistd.h>
void sleep(int time) { usleep(time * 1000); }
#elifdef _WIN32
#include <windows.h>
void sleep(int time) { Sleep(time); }
#endif

const char 
	*TTY_CLEAR = "\x1b[H\x1b[2J",
	*TTY_RED = "\x1b[31;49m",
	*TTY_GREEN = "\x1b[32;49m",
	*TTY_DEFAULT = "\x1b[39;49m"
;

const int 
	STATE_WORD_SELECTION = 0,
	STATE_CHAR_SELECTION = 1,
	STATE_CHAR_MODIFICATINO = 2,
	STATE_COUNT = 3
;

bool isAsciiLetter(char x) { return (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z'); }

void memcpy(void *dest, const void *src, size_t size) 
{
	if (!dest || !src)
		return;

	for (size_t i = 0; i < size; i++)
		((char*)dest)[i] = ((char*)src)[i];
}

void corrupt(char *text, int percentage) 
{
	if (!text || percentage < 0 || percentage > 100)
		return;

	for (int i = 0; text[i] != 0; i++) 
	{
		if (rand() % 100 >= percentage || !isAsciiLetter(text[i]))
			continue;

		int bit = rand() % 6;

		text[i] ^= 1 << bit;
	}
}

int getstreamsize(std::ifstream &file, size_t offset = 50) 
{
	if (!file.good() || offset == 0)
		return -1;

	size_t oldOffset = file.tellg();

	file.seekg(offset);
	int a = file.peek();
	file.seekg(offset + 1);
	int b = file.peek();

	bool outside = a == EOF && b == EOF;
	size_t step = offset;
	int size;

	while (true) 
	{
		file.seekg(offset);
		a = file.peek();
		file.seekg(offset + 1);
		b = file.peek();

		if (a != EOF && b == EOF) 
		{
			size = offset + 1;
			break;
		}
		else if (a == EOF && b == EOF) 
		{
			offset -= step;

			if (outside) step <<= 1;
			else step >>= 1;

			if (!step) step = 1;

			outside = true;
		}
		else if (a != EOF && b != EOF) 
		{
			offset += step;

			if (outside) step >>= 1;
			else step <<= 1;

			if (!step) step = 1;

			outside = false;
		}
		else 
		{
			size = -1;
			break;
		}
	}

	file.seekg(oldOffset);
	return size;
}

void printText(const char *text, const char *corruptedText, char *workingText, int index, int length) 
{
	if (!text || !corruptedText || !workingText || index < 0 || length <= 0)
		return;

	int max = index + length;

	for (int i = index; i < max; i++) 
	{
		if (!text[i] || !corruptedText[i] || !workingText[i])
			break;

		if (workingText[i] == text[i] && workingText[i] == corruptedText[i])
			std::cout << TTY_DEFAULT << workingText[i];
		else if (workingText[i] == corruptedText[i])
			std::cout << TTY_RED << workingText[i];
		else if (workingText[i] == text[i])
			std::cout << TTY_GREEN << workingText[i];
		else
			std::cout << TTY_DEFAULT << workingText[i];
	}

	std::cout << TTY_DEFAULT;
}

int wordSelectionState(const char *text, const char *corruptedText, char *workingText, int textLength, int *wordStart, int *wordLength) 
{
	printText(text, corruptedText, workingText, 0, textLength);

	int word;
	std::cout << "\n\nEnter the number of the word you wish to inspect: ";
	std::cin >> word;

	if (word < 1)
		return EINVAL;

	word--;
	*wordStart = -1;

	// TODO: better word detection
	for (int i = 0; i < textLength; i++) 
	{
		bool isSpace = workingText[i] == ' ';

		if (isSpace)
			word--;

		if (word == 0 && isSpace) 
		{
			*wordStart = i;
			continue;
		}

		if (word == -1 && isSpace) 
		{
			(*wordStart)++;
			*wordLength = i - *wordStart;
			return 0;
		}
	}

	return EINVAL;
}

int charSelectionState(const char *text, const char *corruptedText, char *workingText, int textLength, int wordStart, int wordLength, int *charIndex) 
{
	printText(text, corruptedText, workingText, 0, textLength);

	std::cout << TTY_DEFAULT << "\nSelected word is: ";

	printText(text, corruptedText, workingText, wordStart, wordLength);

	std::cout << "\n\nEnter the number of the character in this word you wish to inspect (0 to cancel): ";
	std::cin >> *charIndex;

	if (!(*charIndex))
		return ECANCELED;

	return (*charIndex <= wordLength) ? 0 : EINVAL;
}

int charModificationState(const char *text, const char *corruptedText, char *workingText, int textLength, int wordStart, int wordLength, int charIndex, char *newChar) 
{
	printText(text, corruptedText, workingText, 0, textLength);

	std::cout << TTY_DEFAULT << "\nSelected word is: ";

	printText(text, corruptedText, workingText, wordStart, wordLength);

	std::cout << TTY_DEFAULT << "\nSelected char is: ";

	for (int i = 1; i < charIndex; i++)
		std::cout << ' ';

	std::cout << '^';

	std::cout << "\n\nChoose what to change the selected character to: \n0) Cancel\n";

	char x = workingText[wordStart + charIndex - 1];
	for (int i = 0; i < 6; i++)
		std::cout << (i + 1) << ") " << (char)(x ^ (1 << i)) << '\n';

	int newCharIndex;
	std::cout << "Your choice: ";
	std::cin >> newCharIndex;

	if (!newCharIndex)
		return ECANCELED;

	if (newCharIndex < 1 || newCharIndex > 6)
		return EINVAL;

	*newChar = (newCharIndex--) ? (x ^ (1 << newCharIndex)) : 0;

	workingText[wordStart + charIndex - 1] = *newChar;

	return 0;
}

int main() 
{
	std::string path;
	std::cout << "path: ";
	std::getline(std::cin, path);

	std::ifstream file(path);
	if (!file.good()) 
	{
		std::cout << "failed to open file '" << path << "'\n";
		return 1;
	}

	int textLength = getstreamsize(file);
	char *text = new char[textLength + 1];
	file.read(text, textLength);
	text[textLength] = 0;

	char *corruptedText = new char[textLength + 1];
	memcpy(corruptedText, text, textLength + 1);

	double corruptionRate;
	std::cout << "corruption rate (between 0 and 1): ";
	std::cin >> corruptionRate;

	corrupt(corruptedText, corruptionRate * 100.0);

	char *workingText = new char[textLength + 1];
	memcpy(workingText, corruptedText, textLength + 1);

	srand(time(NULL) ^ (time_t)text);
	std::cout << TTY_CLEAR;

	int state = STATE_WORD_SELECTION;
	int wordStart, wordLength, charIndex;
	char newChar;
	bool result;

	for (;;) 
	{
		switch (state) 
		{
			case STATE_WORD_SELECTION:
				switch (wordSelectionState(text, corruptedText, workingText, textLength, &wordStart, &wordLength)) 
				{
					case EINVAL:
						std::cout << TTY_CLEAR;
						std::cout << "invalid input\n";
						continue;
				}
				break;
			case STATE_CHAR_SELECTION:
				switch (charSelectionState(text, corruptedText, workingText, textLength, wordStart, wordLength, &charIndex)) 
				{
					case EINVAL:
						std::cout << TTY_CLEAR;
						std::cout << "invalid input\n";
						continue;
					case ECANCELED:
						std::cout << TTY_CLEAR;
						state--;
						continue;
				}
				break;
			case STATE_CHAR_MODIFICATINO:
				switch (charModificationState(text, corruptedText, workingText, textLength, wordStart, wordLength, charIndex, &newChar)) 
				{
					case EINVAL:
						std::cout << TTY_CLEAR;
						std::cout << "invalid input\n";
						continue;
					case ECANCELED:
						std::cout << TTY_CLEAR;
						state--;
						continue;
				}
				break;
		}

		sleep(200);
		state = (state + 1) % STATE_COUNT;
		std::cout << TTY_CLEAR;
	}

	delete[] text, corruptedText, workingText;
	return 0;
}
