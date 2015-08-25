#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <string>
#include <openssl/ssl.h>

const int TimedOutValue = 300; // 用const替代
#define TIMED_OUT_VALUE 300

extern bool SSLConnectDone;
extern bool plainReadDone;
extern bool SSLReadDone;
extern bool plainWriteDone;
extern bool SSLWriteDone;

struct tReadWritePmt {
	int fd;
	SSL *ssl;
	void *rBuff;
	const void *wBuff;
	int size;
	int len;
	int nread;
}; // 补充类内初始化值？

class HttpClient {
public:
	// fundamental functions
	HttpClient() :
        reqMethod("GET")
    { } // 未列出成员用类内初始化或默认初始化。用类内初始化好处在于新增加成员只需修改一处，不必修改构造函数
    ~HttpClient() = default; // 检查资源是否释放，否则结束进程(Effective C++ Item 08)
    HttpClient(const HttpClient &) = delete; // socket为不可共享或复制的资源，阻止拷贝
    HttpClinet &operator=(cosnt HttpClient &) = delete;

	// set up request (protocol = HTTP/1.1) ====================
	// set Method (default = "GET")
	void setReqMethod(const std::string &);

	// set url (consist of protocol(http/https), domain/ip, filepath)
	void setReqUrl(const std::string &);
	
	std::string getReqHostIp();

	// add Header (default = "")
	void addReqHeader(const std::string &, const std::string &);
	void addReqHeader(const std::string &, int);

	// connect server ==========================================
	// TCP handshake; if useSSL then SSL handshake
	// succeed or throw runtime_error
	void connectServ();

	// send data to server =====================================
	// assemble request head; send to server
	void sendHead();

	// send len bytes in buff to server
	void sendBody(const char *buff, int len);

	// recv data from server ===================================
	// recv response head; extract info
	void recvHead(); // recv response head; extract info

	std::string getRespStatus();
	int getRespContentLength();
	std::string getRespLocation();

	// recv at most size bytes
	int recvBody(char *buff, int size);

	// recv all response body (end with 0)
	void recvBodyToEnd(char *buff, int size);

	// recv all response body but do not save
	void passBodyToEnd();

	// free sockfd, ssl, ssl_ctx ===============================
	void cleanUp();

private:
	std::string reqMethod;
	bool useSSL = false;		// extract from setReqUrl()
	std::string reqHost;		// extract from setReqUrl()
	std::string reqIp;		// extract from setReqUrl()
	std::string reqUrl;		// extract from setReqUrl()
	std::string reqHeaders;

	std::string respStatus;
	int respContentLength = 0;
	std::string respLocation;
	std::string respTransferEncoding;
	std::string respBodyCache; // 接收head时若接收了部分body则先缓存在这里

// network connection descriptor
	int clientfd = -1; // socket descriptor
	SSL_CTX *ctx = nullptr; // ssl context
	SSL *ssl = nullptr; // ssl descriptor
};

#endif

/* runtime_error Message List
 * create_socket_failed_xx
 * gethostbyname_failed_xx_[domain]
 * inet_pton_failed_xx
 * connect_failed_xx
 * plain_write_failed			check errno
 * plain_read_failed			check errno
 * SSL_CTX_new_failed			check error stack
 * SSL_new_failed				check error stack
 * SSL_set_fd_failed			check error stack
 * SSL_connect_failed			SSL_get_error(ret)
 * SSL_write_failed				SSL_get_error(ret)
 * SSL_read_failed				SSL_get_error(ret)
 * HTTP_status_code_error_xxx
 */

