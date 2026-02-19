#include "FileIO.h"

FileIO::FileIO() {}

FileIO::~FileIO() {}

bool FileIO::Open(const string& filename) {
    file_stream.open(filename);

    if (!file_stream.is_open()) {
        cerr << "Errore nell'apertura del file: " << filename << endl;
        return false;
    }
    return true;
}

string FileIO::Read() {
    if (file_stream.is_open()) {
        return string((istreambuf_iterator<char>(file_stream)), istreambuf_iterator<char>());
    }
    return "";
}

string FileIO::ReadLine() {
    string line;
    if (!getline(file_stream, line)) {
        return "";
    }
    return line;
}

bool FileIO::Rewind() {
    if (file_stream.is_open()) {
        file_stream.clear();
        file_stream.seekg(0);
        return true;
    }
    return false;
}

bool FileIO::Close() {
    if (file_stream.is_open()) {
        file_stream.close();
        return true;
    }
    return false;
}