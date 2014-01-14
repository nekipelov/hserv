/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <iostream>

#include <hserv/httpserver.h>
#include <hserv/context.h>

using namespace hserv;

int main(int, char**)
{
    int port = 3000;
    std::string address = "0.0.0.0";

    std::cout << "--> Test HTTP Server started on "
              << "http://" << address << ":" << port
              << std::endl;

    auto handler = [](boost::shared_ptr<Context> context) {
        std::cout << context->request().method() << " " << context->request().uri() << std::endl;

        auto &response = context->response();
        response.setStatus( Response::Ok );
        response.addHeader("Content-type", "text/html" );
        response.setContent("<h1>Hello world!</h1>");

        context->asyncDone();
    };

    HttpServer( address, port, handler ).run();

    return 0;
}
