#pragma once
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

class FileIO
{
public:

	FileIO();
	~FileIO();

	bool Open(const string& filename);
	string Read();
	string ReadLine();
	bool Rewind();
	bool Close();

private:
	ifstream file_stream;
};

