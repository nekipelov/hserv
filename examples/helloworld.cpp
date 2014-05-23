/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <iostream>
#include <boost/lexical_cast.hpp>

#include <hserv/httpserver.h>

using namespace hserv;

void handler(const boost::shared_ptr<Context> &context);

int main(int, char**)
{
    int port = 3000;
    std::string address = "0.0.0.0";
    boost::asio::io_service ioService;

    std::cout << "--> Test HTTP Server started on "
              << "http://" << address << ":" << port
              << std::endl;

    HttpServer server(ioService, address, port, handler);

    server.run();
    ioService.run();

    return 0;
}

void handler(const boost::shared_ptr<Context> &context)
{
//    std::cout << context->request().method() << " " << context->request().uri() << std::endl;

    static const std::string response = "<h1>Hello world!</h1>";

    context->response().setStatus( Response::Ok );
    context->response().addHeader("Content-type", "text/html" );
    context->response().addHeader("Content-length", boost::lexical_cast<std::string>(response.size()));
    context->response().setContent(response);
    context->asyncDone();
}
