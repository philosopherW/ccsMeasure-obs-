#ifndef GOOGLEDRIVECLIENT_H
#define GOOGLEDRIVECLIENT_H

#include <string>

#include "CcsClient.h"

struct GoogleMetadata {
	std::string id;
	std::string downloadUrl;
};

class GoogleDriveClient : public CcsClient {
public:
    // constructor & destructor
    GoogleDriveClient() = default;
	GoogleDriveClient(const std::string &cid, const std::string &csec, const std::string &url) :
        CcsClient(cid, csec), redirectUrl(url) { }

    // authentication
	std::string getAuthUrl() const override;
	std::string authByCode(const std::string &code) override;
	void authByTokenString(const std::string &tokenString) override;
	std::string authByRefreshToken() override;

    // functionality
	void upload(int size, const std::string &cloudPath) const override;
	void download(const std::string &cloudPath) const override;
	void remove(const std::string &cloudPath, bool) const override;

	struct GoogleMetadata getMetadataByPath(const std::string &, bool);

private:
	std::string redirectUrl;
};

#endif
