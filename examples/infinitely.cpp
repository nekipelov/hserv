#include <iostream>
#include <string>

#include <libahttp/server.h>

using namespace libahttp;

int main(int, char**)
{
    int port = 3000;
    std::string address = "0.0.0.0";

    std::cout << "--> Test HTTP Server started on "
              << "http://" << address << ":" << port
              << std::endl;

    auto callback = [](const Request& req, Response &res) {
        std::cout << req.method() << " " << req.uri() << std::endl;

        res.setStatus( Response::Ok );
        res.addHeader("Content-type", "text/html" );
        res.setContent("<h1>Hello world!</h1>");
        res.asyncDone();
    };

    Server( Server::FastCGI, address, port, callback ).run();

    return 0;
}
