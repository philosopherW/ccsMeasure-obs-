#include <iostream>
#include <string>
#include <stdexcept>

#include "BaidupcsClient.h"
#include "HttpClient.h"
#include "Tools.h"

using namespace std;

string BaidupcsClient::getAuthUrl() const
{
	string url = string() + 
        "http://openapi.baidu.com/oauth/2.0/authorize?" +
		"client_id=" + clientId + "&" +
		"response_type=code&" +
		"redirect_uri=oob&" +
		"scope=netdisk";
	return url;
}

string BaidupcsClient::authByCode(const string &code)
{
	HttpClient httpClient;
	httpClient.setReqMethod("POST");
	string url = string("https://openapi.baidu.com/oauth/2.0/token?") +
		"grant_type=authorization_code&" +
		"code=" + code + "&" +
		"client_id=" + clientId + "&" +
		"client_secret=" + clientSecret + "&" +
		"redirect_uri=oob";
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Content-Length", 0);

	httpClient.connectServ();

	httpClient.sendHead();

	httpClient.recvHead();
	char buff[RESP_BODY_SIZE];
	httpClient.recvBodyToEnd(buff, sizeof(buff));

	httpClient.cleanUp();

	accessToken = JsonExtractStr(buff, "access_token");
	refreshToken = JsonExtractStr(buff, "refresh_token");

	return accessToken + ":" + refreshToken;
}

void BaidupcsClient::authByTokenString(const string &tokenString)
{
	int pos = tokenString.find(":");
	accessToken = tokenString.substr(0, pos);
	refreshToken = tokenString.substr(pos+1);
}

string BaidupcsClient::authByRefreshToken()
{
	return "";
}

void BaidupcsClient::upload(int size, const string &cloudPath) const
{
	HttpClient httpClient;
	httpClient.setReqMethod("POST");
	string url = string("https://c.pcs.baidu.com/rest/2.0/pcs/file?") +
		"method=upload&" +
		"access_token=" + accessToken + "&" +
		"path=" + urlEncode(cloudPathRoot + cloudPath) + "&" +
		"ondpu=overwrite";
	httpClient.setReqUrl(url);
	string bound = getRandomBoundary();
	httpClient.addReqHeader("Content-Type", string("multipart/form-data; boundary=")+bound);
	string reqBodyHeadStr = 
		"--" + bound + "\r\n" +
		"Content-Disposition: form-data; name=\"file\"; filename=\"randomfile\"\r\n" +
		"Content-Type: application/octet-stream\r\n" +
		"\r\n";
	string reqBodyTailStr = "\r\n--" + bound + "--";
	char reqBodyBuff[1024];
	randomAl(reqBodyBuff, sizeof(reqBodyBuff));
	httpClient.addReqHeader("Content-Length", reqBodyHeadStr.size()+size+reqBodyTailStr.size());

	httpClient.connectServ();

	httpClient.sendHead();
	httpClient.sendBody(reqBodyHeadStr.c_str(), reqBodyHeadStr.size());
	int nleft = size;
	while (nleft > 0) {
		httpClient.sendBody(reqBodyBuff, MIN(nleft, sizeof(reqBodyBuff)));
		nleft -= MIN(nleft, sizeof(reqBodyBuff));
	}
	httpClient.sendBody(reqBodyTailStr.c_str(), reqBodyTailStr.size());

	httpClient.recvHead();

	httpClient.cleanUp();
}

void BaidupcsClient::download(const string &cloudPath) const
{
	string url;
	// get redirect location
	try {
		HttpClient httpClient;
		url = string("https://d.pcs.baidu.com/rest/2.0/pcs/file?") +
			"method=download&" +
			"access_token=" + accessToken + "&" +
			"path=" + urlEncode(cloudPathRoot + cloudPath);
		httpClient.setReqUrl(url);

		httpClient.connectServ();

		httpClient.sendHead();

		httpClient.recvHead();
		url = httpClient.getRespLocation();

		httpClient.cleanUp();
	} catch (runtime_error err) {
		throw runtime_error(string("preDownload:")+err.what());
	}

	// download file
	try {
		HttpClient httpClient2;
		httpClient2.setReqUrl(url);

		recordTrace(string("BaiduPCS-download-tmp: ")+httpClient2.getReqHostIp());

		httpClient2.connectServ();

		httpClient2.sendHead();

		httpClient2.recvHead();
		httpClient2.passBodyToEnd();

		httpClient2.cleanUp();
	} catch (runtime_error err) {
		throw runtime_error(string("download:")+err.what());
	}
}

void BaidupcsClient::remove(const string &cloudPath) const
{
	HttpClient httpClient;
	httpClient.setReqMethod("POST");
	string url = string("https://pcs.baidu.com/rest/2.0/pcs/file?") +
		"method=delete&" +
		"access_token=" + accessToken + "&" +
		"path=" + urlEncode(cloudPathRoot + cloudPath);
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Content-Length", 0);

	httpClient.connectServ();

	httpClient.sendHead();

	httpClient.recvHead();

	httpClient.cleanUp();
}

