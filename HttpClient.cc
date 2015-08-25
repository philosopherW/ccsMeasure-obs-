// socket
#include <sys/socket.h>
#include <netinet/in.h> // sockaddr_in; htons()
#include <netdb.h> // gethostbyname()
#include <arpa/inet.h> // inet_pton()

#include <unistd.h>

// openssl
#include <openssl/ssl.h>

// misc
#include <string> // library string
#include <errno.h> // errno
#include <string.h> // memset(), strlen()
#include <stdio.h> // snprintf(), sscanf()
#include <stdexcept> // runtime_error
#include <iostream>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#include "HttpClient.h"
#include "Tools.h"

using namespace std;

bool SSLConnectDone;
bool plainReadDone;
bool SSLReadDone;
bool plainWriteDone;
bool SSLWriteDone;

//  HTTP(s) request set up ====================================
void HttpClient::setReqMethod(const string &pReqMethod)
{
	reqMethod = pReqMethod;
}

void HttpClient::setReqUrl(const string &pReqUrl)
{
	// extract protocol
	if (pReqUrl[4] == 's')
		useSSL = true;
	else
		useSSL = false;

	// extract domain/ip
	int startPos = pReqUrl.find("//");
	startPos += 2;
	int len = pReqUrl.find("/", startPos) - startPos;
	string tmpStr = pReqUrl.substr(startPos, len);
	if (isDomain(tmpStr))
		reqHost = tmpStr;
	else
		reqIp = tmpStr;

	// extract filepath
	startPos += len;
	reqUrl = pReqUrl.substr(startPos);
}

string HttpClient::getReqHostIp()
{
	return reqHost+reqIp;
}

void HttpClient::addReqHeader(const string &pKey, const string &pValue)
{
	reqHeaders = reqHeaders + pKey + ": " + pValue + "\r\n";
}

void HttpClient::addReqHeader(const string &pKey, int pValue)
{
	char buff[80];
	snprintf(buff, sizeof(buff), "%d", pValue);
	reqHeaders = reqHeaders + pKey + ": " + buff + "\r\n";
}

// connect to HTTP(s) server ==================================

void *pSSL_connect(void *ssl)
{
	// SSL handshake
	if (SSL_connect((SSL *)ssl) == 1) {
		// success 1; error 0; fatal error <0
		SSLConnectDone = true;
	}
	pthread_exit((void *)0);
}

void eSSL_connect(SSL *ssl)
{
	// set flag
	SSLConnectDone = false;

	// create thread
	pthread_t tid;
	if (pthread_create(&tid, NULL, pSSL_connect, ssl) != 0) {
		throw runtime_error("create_ssl_connect_pthread_failed");
	}

	// check
	int ret;
	time_t tstartConn = time(NULL);
	while (true) {
		ret = pthread_kill(tid, 0);
		if (ret == ESRCH)
			if (SSLConnectDone) {
				pthread_join(tid, NULL);
				return;
			} else
				throw runtime_error("SSL_connect_failed");
		else if (ret == EINVAL)
			throw runtime_error("ssl_connect_pthread_kill_invalid_signal");
		else if ((time(NULL) - tstartConn) >= TIMED_OUT_VALUE)
			throw runtime_error("SSL_connect_timed_out");
	}
}

void HttpClient::connectServ()
{
		cout << "============================= start connectServ() ==============================" << endl; // TEST
	// create socket
		cout << ">> create socket ... "; // TEST
	if ( (clientfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		// success >0; error -1
		throw runtime_error(string("create_socket_failed_")+numToStr(errno));
	}
		cout << "done" << endl; // TEST

	// set server addr
		cout << ">> set server addr ... "; // TEST
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(useSSL?443:80);
	if (reqIp.empty()) {
		struct hostent *hptr;
		if ( (hptr = gethostbyname(reqHost.c_str())) == NULL) {
			throw runtime_error(string("gethostbyname_failed_")+numToStr(h_errno)+"_"+reqHost);
		}
		servaddr.sin_addr = *(struct in_addr *)*(hptr->h_addr_list);
	} else {
		if (inet_pton(AF_INET, reqIp.c_str(), &servaddr.sin_addr) <= 0) {
			// success 1; ivalid format 0; error -1
			throw runtime_error(string("inet_pton_failed_")+numToStr(errno));
		}
	}
		cout << "done" << endl; // TEST

	// TCP handshake
		cout << ">> TCP handshake ... "; // TEST
	if (connect(clientfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
		// success 0; error -1
		throw runtime_error(string("connect_failed_")+numToStr(errno));
	}
		cout << "done" << endl; // TEST

	// SSL set up
	if (useSSL) {
		// SSL initialization
			cout << ">> SSL init ... "; // TEST
		SSL_load_error_strings(); // return void
		SSL_library_init(); // always return 1
			cout << "done" << endl; // TEST

		// create SSL context
			cout << ">> create SSL context ... "; // TEST
		if ( (ctx = SSL_CTX_new(SSLv23_client_method())) == NULL) {
			// success pointer; error NULL
			throw runtime_error("SSL_CTX_new_failed");
		}
			cout << "done" << endl; // TEST

		// create SSL structure
			cout << ">> create SSL ... "; // TEST
		if ( (ssl = SSL_new(ctx)) == NULL) {
			// success pointer; error NULL
			throw runtime_error("SSL_new_failed");
		}
			cout << "done" << endl; // TEST

		// connect ssl with a sockfd
			cout << ">> connect ssl with sockfd ... "; // TEST
		if (SSL_set_fd(ssl, clientfd) == 0) {
			// success 1; error 0
			throw runtime_error("SSL_set_fd_failed");
		}
			cout << "done" << endl; // TEST

		// SSL handshake
			cout << ">> SSL handshake ... "; // TEST
		eSSL_connect(ssl);
			cout << "done" << endl; // TEST
	}
		cout << "============================= out of connectServ() =============================" << endl; // TEST
}

// send data to HTTP(s) server ================================

void *pSSL_write(void *writePmtPtr)
{
	SSL *ssl = ((struct tReadWritePmt *)writePmtPtr)->ssl;
	const void *buff = ((struct tReadWritePmt *)writePmtPtr)->wBuff;
	int nleft = ((struct tReadWritePmt *)writePmtPtr)->len;
	int nwritten; // 一次写入的
	const char *ptr = (const char *)buff; // 当前待写入位置
	while (nleft > 0) {
		if ( (nwritten = SSL_write(ssl, ptr, nleft)) <= 0) {
			// success >0(nwritten); error 0; error <0
			pthread_exit((void *)0);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	SSLWriteDone = true;
	pthread_exit((void *)0);
}

void eSSL_write(SSL *ssl, const void *buff, int len)
{
	// set flag
	SSLWriteDone = false;

	// create thread
	struct tReadWritePmt writePmt;
	writePmt.ssl = ssl;
	writePmt.wBuff = buff;
	writePmt.len = len;

	pthread_t tid;
	int ret;
	if ( (ret = pthread_create(&tid, NULL, pSSL_write, &writePmt)) != 0) {
		throw runtime_error(string("create_SSL_write_pthread_failed_")+numToStr(ret));
	}

	// check
	time_t tstartConn = time(NULL);
	while (true) {
		ret = pthread_kill(tid, 0);
		if (ret == ESRCH)
			if (SSLWriteDone) {
				pthread_join(tid, NULL);
				return;
			} else
				throw runtime_error("SSL_write_failed");
		else if (ret == EINVAL)
			throw runtime_error("SSL_write_pthread_kill_invalid_signal");
		else if ((time(NULL) - tstartConn) >= TIMED_OUT_VALUE)
			throw runtime_error("SSL_write_timed_out");
	}
}

void *pPlain_write(void *writePmtPtr)
{
	int fd = ((struct tReadWritePmt *)writePmtPtr)->fd;
	const void *buff = ((struct tReadWritePmt *)writePmtPtr)->wBuff;
	int nleft = ((struct tReadWritePmt *)writePmtPtr)->len;
	int nwritten; // 一次写入的
	const char *ptr = (const char *)buff; // 当前待写入位置
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				pthread_exit((void *)0);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	plainWriteDone = true;
	pthread_exit((void *)0);
}

void ePlain_write(int fd, const void *buff, int len)
{
	// set flag
	plainWriteDone = false;

	// create thread
	struct tReadWritePmt writePmt;
	writePmt.fd = fd;
	writePmt.wBuff = buff;
	writePmt.len = len;

	pthread_t tid;
	int ret;
	if ( (ret = pthread_create(&tid, NULL, pPlain_write, &writePmt)) != 0) {
		throw runtime_error(string("create_plain_write_pthread_failed_")+numToStr(ret));
	}

	// check
	time_t tstartConn = time(NULL);
	while (true) {
		ret = pthread_kill(tid, 0);
		if (ret == ESRCH)
			if (plainWriteDone) {
				pthread_join(tid, NULL);
				return;
			} else
				throw runtime_error("plain_write_failed");
		else if (ret == EINVAL)
			throw runtime_error("plain_write_pthread_kill_invalid_signal");
		else if ((time(NULL) - tstartConn) >= TIMED_OUT_VALUE)
			throw runtime_error("plain_write_timed_out");
	}
}

// assemble request head; send to server
void HttpClient::sendHead()
{
		cout << "=============================== start sendHead() ===============================" << endl; // TEST
		cout << ">> add Host ... "; // TEST
	if (!reqHost.empty())
		this->addReqHeader("Host", reqHost);
		cout << "done" << endl; // TEST
	string reqHead = reqMethod+" "+reqUrl+" HTTP/1.1\r\n"+reqHeaders+"\r\n";
		cout << "----------------------------------- reqHead ------------------------------------" << endl; // TEST
		cout << reqHead << endl; // TEST
		cout << "--------------------------------------------------------------------------------" << endl; // TEST
	if (useSSL) {
		eSSL_write(ssl, reqHead.c_str(), reqHead.size());
	} else {
		ePlain_write(clientfd, reqHead.c_str(), reqHead.size());
	}
		cout << "============================== out of sendHead() ===============================" << endl; // TEST
}

// send size bytes to server
void HttpClient::sendBody(const char *buff, int len)
{
	if (useSSL) {
		eSSL_write(ssl, buff, len);
	} else {
		ePlain_write(clientfd, buff, len);
	}
}

// recv data from HTTP(s) server ==============================

void *pSSL_read(void *readPmtPtr)
{
	SSL *ssl = ((struct tReadWritePmt *)readPmtPtr)->ssl;
	void *buff = ((struct tReadWritePmt *)readPmtPtr)->rBuff;
	int size = ((struct tReadWritePmt *)readPmtPtr)->size;
	int nread;
	if ( (nread = SSL_read(ssl, buff, size)) <= 0) {
		// success >0(nread); error 0; error <0
		pthread_exit((void *)0);
	} else {
		((struct tReadWritePmt *)readPmtPtr)->nread = nread;
		SSLReadDone = true;
		pthread_exit((void *)0);
	}
}

// 读最多size字节到buff，若成功则返回。若错误或超时则抛出异常。
int eSSL_read(SSL *ssl, void *buff, int size)
{
	// set flag
	SSLReadDone = false;

	// create thread
	struct tReadWritePmt readPmt;
	readPmt.ssl = ssl;
	readPmt.rBuff = buff;
	readPmt.size = size;

	pthread_t tid;
	int ret;
	if ( (ret = pthread_create(&tid, NULL, pSSL_read, &readPmt)) != 0) {
		throw runtime_error(string("create_SSL_read_pthread_failed_")+numToStr(ret));
	}

	// check
	time_t tstartConn = time(NULL);
	while (true) {
		ret = pthread_kill(tid, 0);
		if (ret == ESRCH)
			if (SSLReadDone) {
				pthread_join(tid, NULL);
				return readPmt.nread;
			} else
				throw runtime_error("SSL_read_failed");
		else if (ret == EINVAL)
			throw runtime_error("SSL_read_pthread_kill_invalid_signal");
		else if ((time(NULL) - tstartConn) >= TIMED_OUT_VALUE)
			throw runtime_error("SSL_read_timed_out");
	}
}

void *pPlain_read(void *readPmtPtr)
{
	int fd = ((struct tReadWritePmt *)readPmtPtr)->fd;
	void *buff = ((struct tReadWritePmt *)readPmtPtr)->rBuff;
	int size = ((struct tReadWritePmt *)readPmtPtr)->size;
	int nread;
	if ( (nread = read(fd, buff, size)) < 0) {
		if (errno == EINTR) {
			((struct tReadWritePmt *)readPmtPtr)->nread = 0;
			plainReadDone = true;
			pthread_exit((void *)0);
		}
		else
			pthread_exit((void *)0);
	} else if (nread == 0) {
		pthread_exit((void *)0);
	} else {
		((struct tReadWritePmt *)readPmtPtr)->nread = nread;
		plainReadDone = true;
		pthread_exit((void *)0);
	}
}

// 读最多size字节到buff，若成功则返回。若错误或超时则抛出异常。
int ePlain_read(int fd, void *buff, int size)
{
	// set flag
	plainReadDone = false;

	// create thread
	struct tReadWritePmt readPmt;
	readPmt.fd = fd;
	readPmt.rBuff = buff;
	readPmt.size = size;

	pthread_t tid;
	int ret;
	if ( (ret = pthread_create(&tid, NULL, pPlain_read, &readPmt)) != 0) {
		throw runtime_error(string("create_plain_read_pthread_failed_")+numToStr(ret));
	}

	// check
	time_t tstartConn = time(NULL);
	while (true) {
		ret = pthread_kill(tid, 0);
		if (ret == ESRCH)
			if (plainReadDone) {
				pthread_join(tid, NULL);
				return readPmt.nread;
			} else
				throw runtime_error("plain_read_failed");
		else if (ret == EINVAL)
			throw runtime_error("plain_read_pthread_kill_invalid_signal");
		else if ((time(NULL) - tstartConn) >= TIMED_OUT_VALUE)
			throw runtime_error("plain_read_timed_out");
	}
}

// 接收response head并提取信息
void HttpClient::recvHead()
{
	string respHead = "";
	char buff[65536];
	int nread; // 一次读取字节数
	while (true) {
		if (useSSL) {
			nread = eSSL_read(ssl, buff, sizeof(buff)-1);
		} else {
			nread = ePlain_read(clientfd, buff, sizeof(buff)-1);
		}
		buff[nread] = 0;
		respHead += buff;

		// find "\r\n\r\n"
		int pos;
		if ( (pos = respHead.find("\r\n\r\n")) != -1) {
			// "\r\n\r\n" is found, extract info & return
			// 缓存已读body
			if (respHead.size() - pos > 4) {
				// 读了一部分body
				respBodyCache = respHead.substr(pos+4); // 读到的body缓存起来
				respHead = respHead.substr(0, pos+4);
			}
				cout << "----------------------------------- respHead -----------------------------------" << endl; // TEST
				cout << respHead << endl; // TEST
				cout << "--------------------------------------------------------------------------------" << endl; // TEST
			// extract info
			respStatus = HttpExtractStr(respHead, "Status-Code");
			if (respStatus[0] != '2' && respStatus[0] != '3') {
				throw runtime_error(string("HTTP_status_code_error_")+respStatus);
			}
			respContentLength = HttpExtractNum(respHead, "Content-Length");
			respLocation = HttpExtractStr(respHead, "Location");
			respTransferEncoding = HttpExtractStr(respHead, "Transfer-Encoding");
			return;
		} else {
			// "\r\n\r\n" is not found, continue to read
			; // do nothing
		}
	}
}

string HttpClient::getRespStatus()
{
	return respStatus;
}

int HttpClient::getRespContentLength()
{
	return respContentLength;
}

string HttpClient::getRespLocation()
{
	return respLocation;
}

// 最多读取len字节到buff，返回实际读取字节数
int HttpClient::recvBody(char *buff, int len)
{
	int nread;
	if (!respBodyCache.empty()) {
		// respBodyCache非空则从respBodyCache读
		snprintf(buff, len+1, "%s", respBodyCache.c_str()); // snprintf返回的不是实际读取的字节数，而是想要读的字节数。最多读取第二个参数-1个字节，并在读取的数据后置0。
		nread = strlen(buff); // strlen()实际上是从指定地址查找到第一个0的长度
		respBodyCache = respBodyCache.substr(nread);
	} else {
		// respBodyCache为空则从网络读
		if (useSSL) {
			nread = eSSL_read(ssl, buff, len);
		} else {
			nread = ePlain_read(clientfd, buff, len);
		}
	}
	return nread;
}

// 最多读size-1个字节，结尾写0
void HttpClient::recvBodyToEnd(char *buff, int size)
{
	if (respTransferEncoding == "chunked") {
		// response不含Content-Length，遇到"\r\n\r\n"则传输完成
		int sizeLeft = size - 1; // buff剩余可写空间
		int nread;
		char *ptr = buff;
		while (true) {
			nread = this->recvBody(ptr, sizeLeft);
			ptr += nread;
			sizeLeft -= nread;
			if (sizeLeft <= 0) // 实际上应该不会有小于0出现?
				throw runtime_error("recvBodyToEnd_failed_small_buff");
			// find "\r\n\r\n"
			*ptr = 0;
			string str = buff;
			if (str.find("\r\n\r\n") != -1)
				break;
		}
	} else {
		// response含Content-Length
		int nleft = respContentLength; // 若无内容则length为0
		int sizeLeft = size - 1;
		int nread;
		char *ptr = buff;
		while (nleft > 0) {
			nread = this->recvBody(ptr, sizeLeft);
			ptr += nread;
			sizeLeft -= nread;
			if (sizeLeft <= 0) // 实际上应该不会有小于0出现?
				throw runtime_error("recvBodyToEnd_failed_small_buff");
			nleft -= nread;
		}
		*ptr = 0;
	}
}

// 只读取不记录，输出读取数据量
void HttpClient::passBodyToEnd()
{
	char buff[65536];
	if (respTransferEncoding == "chunked") {
		// response不含Content-Length，遇到"\r\n\r\n"则传输完成
			int total = 0; // TEST
		string str = "";
		int nread;
		while (true) {
			nread = this->recvBody(buff, sizeof(buff)-1);
				total += nread; // TEST
				cout << ">> " << total << " / ?" << endl; // TEST
			// find "\r\n\r\n"
			buff[nread] = 0;
			str += buff;
			if (str.find("\r\n\r\n") != -1)
				break;
			else
				if (str.size() > 3)
					str = str.substr(str.size()-3); // 只保留最后三个字节即可，保证能找到"\r\n\r\n"
		}
	} else {
		// response含Content-Length
		int total = 0;
		int nread;
		while (total < respContentLength) {
			nread = this->recvBody(buff, sizeof(buff)-1);
			total += nread;
				cout << ">> " << total << " / " << respContentLength << endl; // TEST
		}
	}
}

void HttpClient::cleanUp()
{
	if (ssl != NULL) SSL_free(ssl); // return void
	if (ctx != NULL) SSL_CTX_free(ctx); // return void
	if (clientfd != -1)	close(clientfd); // success 0; error -1
}

