#include <iostream>
#include <memory> // shared_ptr

#include "CcsClient.h"
#include "DropboxClient.h"
#include "BaidupcsClient.h"

using namespace std;

shared_ptr<CcsClient> newCcsClient(const string &ccsName)
{
    // return shared_ptr<CcsClient>(new BaidupcsClient("aeq9kqvCGEhR5ADuXu18jLcf", "M4UCjy4TwVxqVe2hLO5pXNXZioXnI8GP", "UniDrive"));
    return shared_ptr<CcsClinet>(new DropboxClient("ymhxpozoy5ulcdn", "9vvxawcza2idlyj"));
}

int main(int argc, char *argv[])
{
    shared_ptr<CcsClient> pclient = newCcsClient("");
    cout << pclient->getAuthUrl() << endl;
    return 0;
}
