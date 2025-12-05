#include <iostream>
#include <fstream>

typedef struct gameheader_t 
{
	const unsigned int seed;
	const int textLength;
	bool isLoaded;
	int state, mistakes, wordStart, wordLength, charIndex;
} gameheader_t;

typedef struct game_t 
{
	union 
	{
		struct 
		{
			const unsigned int seed;
			const int textLength;
			bool isLoaded;
			int state, mistakes, wordStart, wordLength, charIndex;
		};

		gameheader_t header;
	};

	struct 
	{
		const char *const text;
		const char *const corruptedText;
		char *const workingText;
	};
} game_t;

const char 
	*TTY_CLEAR = "\x1b[H\x1b[2J",
	*TTY_RED = "\x1b[31;49m",
	*TTY_GREEN = "\x1b[32;49m",
	*TTY_DEFAULT = "\x1b[39;49m"
;

const int 
	STATE_MENU = -1,
	STATE_WORD_SELECTION = 0,
	STATE_CHAR_SELECTION = 1,
	STATE_CHAR_MODIFICATINO = 2,
	STATE_LOAD_FILE = 3,
	STATE_SAVE_FILE = 4,
	STATE_LOAD = 5,
	STATE_COUNT = 3
;

bool isAsciiLetter(char x) { return (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z'); }
bool isAsciiPrintable(char x) { return x > 0x1f && x < 0x7f; }
bool isAsciiWhiteSpace(char x) { return x == ' ' || x == '\n' || x == '\r' || x == '\t' || x == '\0'; }

void memcpy(void *dest, const void *src, size_t size) 
{
	if (!dest || !src)
		return;

	for (size_t i = 0; i < size; i++)
		((char*)dest)[i] = ((char*)src)[i];
}

int getstreamsize(std::ifstream &file, long long offset = 50) 
{
	if (!file.good())
		return -1;

	long long oldOffset = file.tellg();

	file.seekg(offset);
	int a = file.peek();
	file.seekg(offset + 1);
	int b = file.peek();

	bool outside = a == EOF && b == EOF;
	long long step = offset;
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

		if (offset < 0) 
		{
			outside = true;
			offset = 0;
			step = 1;
		}
	}

	file.seekg(oldOffset);
	return size;
}

int splitWords(const char *str, int *indices, int *lengths) 
{
	if (!str || !indices || !lengths)
		return 0;

	int word = 0, index = 0;

	while (str[index] != 0) 
	{
		for (; isAsciiWhiteSpace(str[index]); index++);
		indices[word] = index;

		for (; !isAsciiWhiteSpace(str[index]); index++);
		lengths[word] = index - indices[word];

		word++;
	}

	return word;
}

void corrupt(char *text, int percentage) 
{
	if (!text || percentage < 0 || percentage > 100)
		return;

	for (int i = 0; text[i] != 0; i++) 
	{
		if (rand() % 100 >= percentage || !isAsciiLetter(text[i]))
			continue;

		char x;

		do 
		{
			int bit = rand() % 6;
			x = text[i] ^ (1 << bit);
		} while (!isAsciiPrintable(x) || x == ' ');

		text[i] = x;
	}
}

void printText(game_t &game, int index, int length) 
{
	if (!game.text || !game.corruptedText || !game.workingText || index < 0 || length <= 0)
		return;

	int max = index + length;

	for (int i = index; i < max; i++) 
	{
		if (!game.text[i] || !game.corruptedText[i] || !game.workingText[i])
			break;

		if (game.workingText[i] == game.text[i] && game.workingText[i] == game.corruptedText[i])
			std::cout << TTY_DEFAULT << game.workingText[i];
		else if (game.workingText[i] == game.corruptedText[i])
			std::cout << TTY_RED << game.workingText[i];
		else if (game.workingText[i] == game.text[i])
			std::cout << TTY_GREEN << game.workingText[i];
		else if (game.workingText[i] != game.corruptedText[i] && game.workingText[i] != game.text[i])
			std::cout << TTY_RED << game.workingText[i];
		else
			std::cout << TTY_DEFAULT << game.workingText[i];
	}

	std::cout << TTY_DEFAULT;
}

int unload(game_t &game) 
{
	if (!game.isLoaded)
		return ENODATA;

	if (game.text)
		delete[] game.text;

	if (game.corruptedText)
		delete[] game.corruptedText;

	if (game.workingText)
		delete[] game.workingText;

	game.isLoaded = false;

	return 0;
}

int load(game_t &game) 
{
	if (game.isLoaded)
		unload(game);

	std::string path;
	std::cout << "path: ";
	std::getline(std::cin >> std::ws, path);

	std::ifstream file(path);

	if (!file.good())
		return ENOENT;

	double corruptionRate;
	std::cout << "corruption rate (between 0 and 1): ";
	std::cin >> corruptionRate;

	if (corruptionRate < 0.0 || corruptionRate > 1.0)
		return EINVAL;

	int textLength = getstreamsize(file);
	char *text = new char[textLength + 1];
	file.read(text, textLength);
	text[textLength] = 0;

	char *corruptedText = new char[textLength + 1];
	memcpy(corruptedText, text, textLength + 1);

	corrupt(corruptedText, corruptionRate * 100.0);

	char *workingText = new char[textLength + 1];
	memcpy(workingText, corruptedText, textLength + 1);

	gameheader_t header = 
	{
		.seed = (unsigned int)time(NULL),
		.textLength = textLength,
		.state = STATE_WORD_SELECTION,
		.mistakes = 0,
		.wordStart = -1,
		.wordLength = -1,
		.charIndex = -1
	};

	memcpy(&game.header, &header, sizeof(header));

	*(char**)(&game.text) = text;
	*(char**)(&game.corruptedText) = corruptedText;
	*(char**)(&game.workingText) = workingText;

	game.isLoaded = true;

	return 0;
}

int loadfile(game_t &game) 
{
	if (game.isLoaded)
		unload(game);

	std::string path;
	std::cout << "path: ";
	std::getline(std::cin >> std::ws, path);

	std::ifstream file(path);

	if (!file.good())
		return ENOENT;

	file.read((char*)(&game.header), sizeof(gameheader_t));

	*(char**)(&game.text) = new char[game.textLength + 1];
	*(char**)(&game.corruptedText) = new char[game.textLength + 1];
	*(char**)(&game.workingText) = new char[game.textLength + 1];

	file.read((char*)game.text, game.textLength + 1);
	file.read((char*)game.corruptedText, game.textLength + 1);
	file.read((char*)game.workingText, game.textLength + 1);

	game.isLoaded = true;

	return 0;
}

int savefile(const game_t &game) 
{
	if (!game.isLoaded)
		return ENODATA;

	std::string path;
	std::cout << "path: ";
	std::getline(std::cin >> std::ws, path);

	std::ofstream file(path);

	if (!file.good())
		return ENOENT;

	file.write((const char*)(&game.header), sizeof(gameheader_t));

	file.write(game.text, game.textLength + 1);
	file.write(game.corruptedText, game.textLength + 1);
	file.write(game.workingText, game.textLength + 1);

	return 0;
}

int menuState(game_t &game) 
{
	std::cout << "what do you want to do?\n";
	std::cout << "1) load game from stdin (create a new game)\n";
	std::cout << "2) load game save file\n";
	std::cout << "3) save current game\n";
	std::cout << "4) exit game\n";
	std::cout << "choice: ";

	int x;
	std::cin >> x;
	std::cout << '\n';

	switch (x) 
	{
		case 1:
			game.state = STATE_LOAD;
			break;
		case 2:
			game.state = STATE_LOAD_FILE;
			break;
		case 3:
			game.state = STATE_SAVE_FILE;
			break;
		case 4:
			return ECANCELED;
		default:
			return EINVAL;
	}

	return 0;
}

int wordSelectionState(game_t &game) 
{
	printText(game, 0, game.textLength);

	int word;
	std::cout << "\n\nEnter the number of the word you wish to inspect (0 to cancel): ";
	std::cin >> word;

	word--;

	if (word == -1)
		return ECANCELED;

	if (word < 0)
		return EINVAL;

	int indices[game.textLength], lengths[game.textLength];

	if (word >= splitWords(game.workingText, indices, lengths))
		return EINVAL;

	game.wordStart = indices[word];
	game.wordLength = lengths[word];

	return 0;
}

int charSelectionState(game_t &game) 
{
	printText(game, 0, game.textLength);

	std::cout << TTY_DEFAULT << "\nSelected word is: ";

	printText(game, game.wordStart, game.wordLength);

	std::cout << "\n\nEnter the number of the character in this word you wish to inspect (0 to cancel): ";
	std::cin >> game.charIndex;

	if (!game.charIndex)
		return ECANCELED;

	return (game.charIndex >= 1 && game.charIndex <= game.wordLength) ? 0 : EINVAL;
}

int charModificationState(game_t &game) 
{
	printText(game, 0, game.textLength);

	std::cout << TTY_DEFAULT << "\nSelected word is: ";

	printText(game, game.wordStart, game.wordLength);

	std::cout << TTY_DEFAULT << "\nSelected char is: ";

	for (int i = 1; i < game.charIndex; i++)
		std::cout << ' ';

	std::cout << '^';

	std::cout << "\n\nChoose what to change the selected character to: \n0) Cancel\n";

	int textOffset = game.wordStart + game.charIndex - 1;

	char x = game.workingText[textOffset];
	for (int i = 0; i < 6; i++)
		std::cout << (i + 1) << ") " << (char)(x ^ (1 << i)) << '\n';

	int newCharIndex;
	std::cout << "Your choice: ";
	std::cin >> newCharIndex;

	if (!newCharIndex)
		return ECANCELED;

	if (newCharIndex < 1 || newCharIndex > 6)
		return EINVAL;

	x = (newCharIndex--) ? (x ^ (1 << newCharIndex)) : 0;

	if (!isAsciiPrintable(x) || x == ' ')
		return EAGAIN;

	game.workingText[textOffset] = x;

	if (game.text[textOffset] != x)
		game.mistakes++;

	return 0;
}

int run(game_t &game) 
{
	std::cout << TTY_CLEAR;

	srand(game.seed);

	for (;;) 
	{
		bool winning = game.isLoaded;

		for (int i = 0; winning && i < game.textLength; i++)
			if (game.text[i] != game.workingText[i])
				winning = false;

		if (winning) 
		{
			std::cout << TTY_CLEAR;
			printText(game, 0, game.textLength);
			std::cout << "\n\nCongratulations! You won! You made only " << game.mistakes << ((game.mistakes == 1) ? " mistake!\n" : " mistakes!\n");
			break;
		}

		switch (game.state) 
		{
			case STATE_MENU:
				switch (menuState(game)) 
				{
					case EINVAL:
						std::cout << TTY_CLEAR;
						std::cout << "invalid input, try again\n\n";
						break;
					case ECANCELED:
						return 0;
				}
				continue;
			case STATE_LOAD:
				switch (load(game)) 
				{
					case ENOENT:
						std::cout << TTY_CLEAR;
						std::cout << "failed to open file, try again\n\n";
						unload(game);
						continue;
					case EINVAL:
						std::cout << TTY_CLEAR;
						std::cout << "invalid input, try again\n\n";
						unload(game);
						continue;
				}
				game.state = STATE_WORD_SELECTION;
				return run(game);
			case STATE_LOAD_FILE:
				switch (loadfile(game)) 
				{
					case ENOENT:
						std::cout << TTY_CLEAR;
						std::cout << "failed to open file, try again\n\n";
						unload(game);
						continue;
				}
				game.state = STATE_WORD_SELECTION;
				return run(game);
			case STATE_SAVE_FILE:
				switch (savefile(game)) 
				{
					case ENOENT:
						std::cout << TTY_CLEAR;
						std::cout << "failed to open file, try again\n";
						unload(game);
						continue;
					case ENODATA:
						std::cout << TTY_CLEAR;
						std::cout << "cannot save uninitialized game\n"; // fatal
						return 1;
				}
				std::cout << TTY_CLEAR;
				game.state = STATE_MENU;
				continue;
			case STATE_WORD_SELECTION:
				switch (wordSelectionState(game)) 
				{
					case EINVAL:
						std::cout << TTY_CLEAR;
						std::cout << "invalid input, try again\n\n";
						continue;
					case ECANCELED:
						std::cout << TTY_CLEAR;
						game.state--;
						continue;
				}
				break;
			case STATE_CHAR_SELECTION:
				switch (charSelectionState(game)) 
				{
					case EINVAL:
						std::cout << TTY_CLEAR;
						std::cout << "invalid input, try again\n\n";
						continue;
					case ECANCELED:
						std::cout << TTY_CLEAR;
						game.state--;
						continue;
				}
				break;
			case STATE_CHAR_MODIFICATINO:
				switch (charModificationState(game)) 
				{
					case EINVAL:
						std::cout << TTY_CLEAR;
						std::cout << "invalid input, try again\n\n";
						continue;
					case EAGAIN:
						std::cout << TTY_CLEAR;
						std::cout << "the selected character is non-printable or whitespace, try again\n\n";
						continue;
					case ECANCELED:
						std::cout << TTY_CLEAR;
						game.state--;
						continue;
				}
				break;
		}

		game.state = (game.state + 1) % STATE_COUNT;
		std::cout << TTY_CLEAR;
	}

	return 0;
}

int main() 
{
	game_t game = {0};
	game.state = STATE_MENU;

	return run(game);
}
