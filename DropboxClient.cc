#include <iostream>
#include <string>

#include "DropboxClient.h"
#include "HttpClient.h"
#include "Tools.h"

using namespace std;

string DropboxClient::getAuthUrl() const
{
	string url = string("https://www.dropbox.com/1/oauth2/authorize?") +
		"response_type=code&" +
		"client_id=" + clientId;
	return url;
}

string DropboxClient::authByCode(const string &code)
{
	HttpClient httpClient;
	httpClient.setReqMethod("POST");
	string url = "https://api.dropbox.com/1/oauth2/token";
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Content-Type", "application/x-www-form-urlencoded");
	string reqBodyStr = string("") +
		"code=" + code + "&" +
		"grant_type=authorization_code&" +
		"client_id=" + clientId + "&" +
		"client_secret=" + clientSecret;
	httpClient.addReqHeader("Content-Length", reqBodyStr.size());

	httpClient.connectServ();

	httpClient.sendHead();
	httpClient.sendBody(reqBodyStr.c_str(), reqBodyStr.size());

	httpClient.recvHead();
	char buff[RESP_BODY_SIZE];
	httpClient.recvBodyToEnd(buff, sizeof(buff));

	httpClient.cleanUp();

	accessToken = JsonExtractStr(buff, "access_token");

	return accessToken;
}

void DropboxClient::authByTokenString(const string &tokenString)
{
	accessToken = tokenString;
}

string DropboxClient::authByRefreshToken()
{
	return "";
}

void DropboxClient::upload(int size, const string &cloudPath) const
{
	HttpClient httpClient;
	httpClient.setReqMethod("PUT");
	string url = string("https://api-content.dropbox.com/1/files_put/auto") + cloudPath + "?" +
		"overwrite=true";
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Authorization", string("Bearer ") + accessToken);
	char reqBodyBuff[1024]; randomAl(reqBodyBuff, sizeof(reqBodyBuff));
	httpClient.addReqHeader("Content-Length", size);

	httpClient.connectServ();

	httpClient.sendHead();
	int nleft = size;
	while (nleft > 0) {
		httpClient.sendBody(reqBodyBuff, MIN(nleft, sizeof(reqBodyBuff)));
		nleft -= MIN(nleft, sizeof(reqBodyBuff));
	}

	httpClient.recvHead();

	httpClient.cleanUp();
}

void DropboxClient::download(const string &cloudPath) const
{
	HttpClient httpClient;
	string url = string("https://api-content.dropbox.com/1/files/auto") + cloudPath;
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Authorization", string("Bearer ") + accessToken);

	httpClient.connectServ();

	httpClient.sendHead();

	httpClient.recvHead();
	httpClient.passBodyToEnd();

	httpClient.cleanUp();
}

void DropboxClient::remove(const string &cloudPath) const
{
	HttpClient httpClient;
	httpClient.setReqMethod("POST");
	string url = "https://api.dropbox.com/1/fileops/delete";
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Authorization", string("Bearer ") + accessToken);
	httpClient.addReqHeader("Content-Type", "application/x-www-form-urlencoded");
	string reqBodyStr = string("") +
		"root=auto&" +
		"path=" + cloudPath;
	httpClient.addReqHeader("Content-Length", reqBodyStr.size());

	httpClient.connectServ();

	httpClient.sendHead();
	httpClient.sendBody(reqBodyStr.c_str(), reqBodyStr.size());

	httpClient.recvHead();

	httpClient.cleanUp();
}

