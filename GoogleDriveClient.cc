#include <iostream>
#include <string>
#include <stdexcept>

#include "GoogleDriveClient.h"
#include "HttpClient.h"
#include "Tools.h"

using namespace std;

GoogleDriveClient::GoogleDriveClient(const string &id, const string &secret, const string &url)
{
	clientId = id;
	clientSecret = secret;
	redirectUrl = url;
}

string GoogleDriveClient::getAuthUrl()
{
	string url = string("https://accounts.google.com/o/oauth2/auth?") +
		"response_type=code&" +
		"client_id=" + clientId + "&" +
		"redirect_uri=" + redirectUrl + "&" +
		"scope=https://www.googleapis.com/auth/drive&" +
		"access_type=offline";
	return url;
}

string GoogleDriveClient::authByCode(const string &code)
{
	HttpClient httpClient;
	httpClient.setReqMethod("POST");
	string url = "https://accounts.google.com/o/oauth2/token";
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Content-Type", "application/x-www-form-urlencoded");
	string reqBodyStr = string("") +
		"code=" + code + "&" +
		"client_id=" + clientId + "&" +
		"client_secret=" + clientSecret + "&" +
		"redirect_uri=" + redirectUrl + "&" +
		"grant_type=authorization_code";
	httpClient.addReqHeader("Content-Length", reqBodyStr.size());

	httpClient.connectServ();

	httpClient.sendHead();
	httpClient.sendBody(reqBodyStr.c_str(), reqBodyStr.size());

	httpClient.recvHead();
	char buff[RESP_BODY_SIZE];
	httpClient.recvBodyToEnd(buff, sizeof(buff));

	httpClient.cleanUp();

	accessToken = JsonExtractStr(buff, "access_token");
	refreshToken = JsonExtractStr(buff, "refresh_token");

	return accessToken + ":" + refreshToken;
}

void GoogleDriveClient::authByTokenString(const string &tokenString)
{
	int pos = tokenString.find(":");
	accessToken = tokenString.substr(0, pos);
	refreshToken = tokenString.substr(pos+1);
}

string GoogleDriveClient::authByRefreshToken()
{
	HttpClient httpClient;
	httpClient.setReqMethod("POST");
	string url = "https://accounts.google.com/o/oauth2/token";
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Content-Type", "application/x-www-form-urlencoded");
	string reqBodyStr = string("") +
		"refresh_token=" + refreshToken + "&" +
		"client_id=" + clientId + "&" +
		"client_secret=" + clientSecret + "&" +
		"grant_type=refresh_token";
	httpClient.addReqHeader("Content-Length", reqBodyStr.size());

	httpClient.connectServ();

	httpClient.sendHead();
	httpClient.sendBody(reqBodyStr.c_str(), reqBodyStr.size());

	httpClient.recvHead();
	char buff[RESP_BODY_SIZE];
	httpClient.recvBodyToEnd(buff, sizeof(buff));

	httpClient.cleanUp();

	accessToken = JsonExtractStr(buff, "access_token");

	return accessToken + ":" + refreshToken;
}

void GoogleDriveClient::upload(int size, const string &cloudPath)
{
	HttpClient httpClient;
	httpClient.setReqMethod("POST");
	string url = string("https://www.googleapis.com/upload/drive/v2/files?") +
		"access_token=" + accessToken + "&" +
		"uploadType=multipart";
	httpClient.setReqUrl(url);
	string bound = getRandomBoundary();
	httpClient.addReqHeader("Content-Type", string("multipart/related; boundary=\"")+bound+"\"");
	string reqBodyHeadStr = 
		"--" + bound + "\r\n" +
		"Content-Type: application/json; charset=UTF-8\r\n" +
		"\r\n" +
		"{" +
		"\"title\":\"" + getFileName(cloudPath) + "\"," +
		"\"parents\":[{\"id\":\"" + this->getMetadataByPath(getDirectoryName(cloudPath), true).id + "\"}]" +
		"}\r\n" +
		"--" + bound + "\r\n" +
		"Content-Type: application/octet-stream\r\n" +
		"\r\n";
	string reqBodyTailStr = "\r\n--" + bound + "--\r\n";
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

void GoogleDriveClient::download(const string &cloudPath)
{
	HttpClient httpClient;
	string url = this->getMetadataByPath(cloudPath, false).downloadUrl;
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Authorization", string("Bearer ")+accessToken);

	recordTrace(string("GoogleDrive-download-tmp: ")+httpClient.getReqHostIp());

	httpClient.connectServ();

	httpClient.sendHead();

	httpClient.recvHead();
	httpClient.passBodyToEnd();

	httpClient.cleanUp();
}

void GoogleDriveClient::remove(const string &cloudPath, bool isFolder)
{
	HttpClient httpClient;
	httpClient.setReqMethod("DELETE");
	string url = string("https://www.googleapis.com/drive/v2/files/") + getMetadataByPath(cloudPath, isFolder).id;
	httpClient.setReqUrl(url);
	httpClient.addReqHeader("Authorization", string("Bearer ")+accessToken);

	httpClient.connectServ();

	httpClient.sendHead();

	httpClient.recvHead();

	httpClient.cleanUp();
}

struct GoogleMetadata GoogleDriveClient::getMetadataByPath(const string &cloudPath, bool isFolder)
{
	if (cloudPath == "/"){
		struct GoogleMetadata ret;
		ret.id = "root";
		ret.downloadUrl = "";
		return ret;
	} else {
		string parentId = getMetadataByPath(getDirectoryName(cloudPath), true).id;

		string respBodyStr = "";
		try {
			HttpClient httpClient;
			string url = string("https://www.googleapis.com/drive/v2/files?") +
				"access_token=" + accessToken + "&" +
				"q=" + urlEncode("'" + parentId + "' in parents and title = '" + getFileName(cloudPath) + "' and mimeType " + (isFolder ? "=" : "!=") + " 'application/vnd.google-apps.folder' and trashed = false");
			httpClient.setReqUrl(url);

			httpClient.connectServ();

			httpClient.sendHead();

			httpClient.recvHead();
			int nleft = httpClient.getRespContentLength();
			int nread;
			char buff[65536];
			while (nleft > 0) {
				nread = httpClient.recvBody(buff, sizeof(buff)-1);
				nleft -= nread;
				buff[nread] = 0;
				respBodyStr += buff;
			}

			httpClient.cleanUp();
		} catch (runtime_error err) {
			throw(string("getMetadata:")+err.what());
		}

		// 不管重名文件，直接返回第一个搜索到的id字段和downloadUrl字段的值
		// 若文件不存在则返回的列表为空，相应的id和downloadUrl解析都返回""
		struct GoogleMetadata ret;
		ret.id = JsonExtractStr(respBodyStr, "id");
		ret.downloadUrl = JsonExtractStr(respBodyStr, "downloadUrl");
		if (ret.id.empty() && ret.downloadUrl.empty())
			throw runtime_error(string("file_not_exist_")+cloudPath);
		return ret;
	}
}

