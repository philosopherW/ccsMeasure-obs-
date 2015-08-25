#ifndef CCSCLIENT_H
#define CCSCLIENT_H

#include <string>

class CcsClient {
public:
    // 四个基本函数(若不给出，则编译器会自动生成的四个基本函数。注意自动生成的构造函数无形参)
    CcsClient(const std::string &cid, const std::string &csec) :
        clientId(cid),
        clientSecret(csec),
        accessToken(),
        refreshToken()
    { }
    virtual ~CcsClient() = default;
    // 防止隐式生成copy constructor和copy assignment(是否阻止拷贝要看类表示的是实体物件还是抽象物件，实体物件如房子车子逻辑上不能拷贝，抽象物件如坐标对逻辑上可以拷贝)
    CcsClient(const CcsClient &) = delete;
    CcsClient &operator=(const CcsClient &) = delete;

    // authentication
    virtual std::string getAuthUrl() const = 0;
    virtual std::string authByCode(const std::string &code) = 0;
    virtual void authByTokenString(const std::string &tokenString) = 0;
    virtual std::string authByRefreshToken() = 0;

    // functionality
    virtual void upload(int size, const std::string &cloudPath) const = 0;
    virtual void download(const std::string &cloudPath) const = 0;
    virtual void remove(const std::string &cloudPath) const = 0;

protected:
    // common field
    std::string clientId;
    std::string clientSecret;
    std::string accessToken;
    std::string refreshToken;
};

#endif
