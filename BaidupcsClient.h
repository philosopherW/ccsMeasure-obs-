#ifndef BAIDUPCSCLIENT_H
#define BAIDUPCSCLIENT_H

#include <string>
#include "CcsClient.h"

class BaidupcsClient : public CcsClient {
public:
    // constructor & destructor
    BaidupcsClient() = default;
	BaidupcsClient(const std::string &cid, const std::string &csec, const std::string &appName) :
        CcsClient(cid, csec), cloudPathRoot("/apps/" + appName) { }

    // authentication
	std::string getAuthUrl() const override;
	std::string authByCode(const std::string &code) override;
	void authByTokenString(const std::string &tokenString) override;
	std::string authByRefreshToken() override;

    // functionality
	void upload(int size, const std::string &cloudPath) const override;
	void download(const std::string &cloudPath) const override;
	void remove(const std::string &cloudPath) const override;

private:
	std::string cloudPathRoot;
};

#endif
