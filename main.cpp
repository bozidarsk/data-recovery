#include <iostream>
#include <fstream>

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

int main() 
{
	std::string path;
	std::getline(std::cin, path);

	std::ifstream file(path);
	if (!file.good()) 
	{
		std::cout << "failed to open file\n";
		return 1;
	}

	int textLength = getstreamsize(file);
	char *text = new char[textLength + 1];
	file.read(text, textLength);
	text[textLength] = 0;

	delete[] text;
	return 0;
}
