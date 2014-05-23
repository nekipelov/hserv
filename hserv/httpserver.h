/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_HTTPSERVER_H
#define HSERV_HTTPSERVER_H

#include <string>
#include <boost/function.hpp>

#include <hserv/impl/httpserverimpl.h>
#include <hserv/serverinterface.h>
#include <hserv/context.h>

namespace hserv {

class ServerImpl;
class Connection;

// The HTTP server implementation class.
class HttpServer : public ServerInterface
{
public:
    HttpServer(boost::asio::io_service &ioService,
               const std::string &address, int port,
               const boost::function<void(const boost::shared_ptr<Context> &)> &callback)
        : impl(ioService, address, port, callback)
    {
    }

    ~HttpServer() {
    }

    virtual void run() {
        impl.run();
    }

    virtual void stop() {
        impl.stop();
    }

private:
    HttpServerImpl impl;
};

} // namespace libahttp

#endif // HSERV_HTTPSERVER_H
