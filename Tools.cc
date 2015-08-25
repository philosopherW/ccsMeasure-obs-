#include <string>
#include <stdio.h>
#include <ctype.h> // isalnum()
#include <iostream>
#include <stdlib.h> // rand()
#include <time.h> // time()
#include <sys/time.h> // gettimeofday()
#include <unistd.h>
#include <fstream>

#include "Tools.h"

using namespace std;

string HttpExtractStr(const string &httpStr, const string &key)
{
	int startPos, len;
	if (key == "Status-Code") {
		startPos = httpStr.find(" ") + 1;
		len = httpStr.find(" ", startPos) - startPos;
		return httpStr.substr(startPos, len);
	} else {
		string k = string("\r\n") + key + ":";
		if ( (startPos = httpStr.find(k)) != -1) {
			startPos = httpStr.find(":", startPos);
			startPos += 1;
			while (httpStr[startPos] == ' ')
				++startPos;
			len = httpStr.find("\r\n", startPos) - startPos;
			return httpStr.substr(startPos, len);
		} else {
			return "";
		}
	}
}

int HttpExtractNum(const string &httpStr, const string &key)
{
	int startPos, len;
	string k = string("\r\n") + key + ":";
	if ( (startPos = httpStr.find(k)) != -1) {
		startPos = httpStr.find(":", startPos);
		startPos += 1;
		while (httpStr[startPos] == ' ')
			++startPos;
		len = httpStr.find("\r\n", startPos) - startPos;
		string valueStr = httpStr.substr(startPos, len);
		int value;
		sscanf(valueStr.c_str(), "%d", &value);
		return value;
	} else {
		return 0;
	}
}

string JsonExtractStr(const string &jsonStr, const string &key)
{
	string k = string("\"") + key + "\"";
	int startPos = jsonStr.find(k); if (startPos == -1) return "";
	startPos = jsonStr.find(":", startPos);
	startPos = jsonStr.find("\"", startPos);
	startPos += 1;
	int len = jsonStr.find("\"", startPos) - startPos;
	return jsonStr.substr(startPos, len);
}

string JsonExtractStrFrom(const string &jsonStr, const string &key, int &pos)
{
	string k = string("\"") + key + "\"";
	int startPos = jsonStr.find(k, pos); if (startPos == -1) return "";
	startPos = jsonStr.find(":", startPos);
	startPos = jsonStr.find("\"", startPos);
	startPos += 1;
	int len = jsonStr.find("\"", startPos) - startPos;
	pos = startPos + len;
	return jsonStr.substr(startPos, len);
}

unsigned char ToHex(unsigned char x)
{
	return x > 9 ? x + 55 : x + 48;
}

string urlEncode(const string &str)
{
	string retStr = "";
	int len = str.size();
	for (int i = 0; i < len; ++i) {
		//cout << str[i]; // TEST
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			retStr += str[i];
		else if (str[i] == ' ')
			retStr += "+";
		else {
			retStr += "%";
			retStr += ToHex((unsigned char)str[i] >> 4);
			retStr += ToHex((unsigned char)str[i] % 16);
		}
	}
	return retStr;
}

string getRandomBoundary()
{
	string sourceStr = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	string retStr = "";

	srand((int)time(0));
	for (int i = 0; i < 40; ++i)
		retStr += sourceStr[rand()%62];

	return retStr;
}

void randomAl(char *buff, int size)
{
	string sourceStr = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	srand((int)time(0));
	for (int i = 0; i < size; ++i)
		buff[i] = sourceStr[rand()%52];
}

bool isDomain(const string &str)
{
	for (int i = 0; i < str.size(); ++i)
		if (!isdigit(str[i]) && str[i] != '.')
			return true;
	return false;
}

double gettime()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + (double)t.tv_usec/1000000;
}

string getFileName(const string &path)
{
	int startPos = path.rfind("/");
	startPos += 1;
	return path.substr(startPos);
}

string getDirectoryName(const string &path)
{
	int len = path.rfind("/");
	if (len == 0)
		return "/";
	else
		return path.substr(0, len);
}

string numToStr(int num)
{
	char buff[80];
	snprintf(buff, sizeof(buff), "%d", num);
	return string(buff);
}

string numToStr(double num)
{
	char buff[80];
	snprintf(buff, sizeof(buff), "%f", num);
	return string(buff);
}

int strToNum(const string &str)
{
	int ret;
	sscanf(str.c_str(), "%d", &ret);
	return ret;
}

void recordTrace(const string &info)
{
	char buff[65];
	gethostname(buff, sizeof(buff)-1);
	string filePath = string("latency_") + buff + ".log";

	time_t t = time(NULL);
	struct tm *nowTime = gmtime(&t);
	string timeStr =
		numToStr(nowTime->tm_year + 1900) + " " +
		numToStr(nowTime->tm_mon + 1) + " " +
		numToStr(nowTime->tm_mday) + " " +
		numToStr(nowTime->tm_hour) + " " +
		numToStr(nowTime->tm_min) + " " +
		numToStr(nowTime->tm_sec) + " ";

	ofstream fout(filePath.c_str(), fstream::app);
	fout << timeStr << info << endl;
	fout.close();
}

