#ifndef DROPBOXCLIENT_H
#define DROPBOXCLIENT_H

#include <string>

#include "CcsClient.h"

class DropboxClient : public CcsClient {
public:
    // constructor & destructor
    DropboxClient() = default;
	DropboxClient(const std::string &cid, const std::string &csec) :
        CcsClient(cid, csec) { }

    // authentication
	std::string getAuthUrl() const override;
	std::string authByCode(const std::string &code) override;
	void authByTokenString(const std::string &tokenString) override;
	std::string authByRefreshToken() override;

    // functionality
	void upload(int size, const std::string &cloudPath) const override;
	void download(const std::string &cloudPath) const override;
	void remove(const std::string &cloudPath) const override;
};

#endif 
