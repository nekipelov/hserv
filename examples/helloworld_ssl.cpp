/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <iostream>

#include <hserv/httpsserver.h>

using namespace hserv;

void handler(const boost::shared_ptr<Context> &context);
std::string getPassword();

int main(int, char**)
{
    int port = 3000;
    std::string address = "0.0.0.0";
    boost::asio::ssl::context context(boost::asio::ssl::context::sslv23);

    context.set_options(boost::asio::ssl::context::default_workarounds);
    context.set_password_callback(boost::bind(&getPassword));
    context.use_certificate_chain_file("server.pem");
    context.use_private_key_file("server.pem", boost::asio::ssl::context::pem);

    std::cout << "--> Test HTTP Server started on "
              << "http://" << address << ":" << port
              << std::endl;

    HttpsServer( address, port, context, handler ).run();

    return 0;
}

void handler(const boost::shared_ptr<Context> &context)
{
    std::cout << context->request().method() << " " << context->request().uri() << std::endl;

    context->response().setStatus( Response::Ok );
    context->response().addHeader("Content-type", "text/html" );
    context->response().setContent("<h1>Hello world!</h1>");
    context->asyncDone();
}

std::string getPassword()
{
    std::cerr << "get pass" << std::endl;
    return "test";
}
