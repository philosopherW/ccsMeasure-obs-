#ifndef TOOLS_H
#define TOOLS_H

#include <string>

#define MIN(a, b) (a>b ? b : a)
#define RESP_BODY_SIZE 4096

using namespace std;

string HttpExtractStr(const string &, const string &);
int HttpExtractNum(const string &, const string &);
string JsonExtractStr(const string &, const string &);
string JsonExtractStrFrom(const string &, const string &, int &);

string urlEncode(const string &);

string getRandomBoundary();

void randomAl(char *, int);

bool isDomain(const string &);

double gettime();

string getFileName(const string &);
string getDirectoryName(const string &);

string numToStr(int);
string numToStr(double);
int strToNum(const string &);

void recordTrace(const string &);

#endif
