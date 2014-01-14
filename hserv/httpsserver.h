/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_HTTPSSERVER_H
#define HSERV_HTTPSSERVER_H

#include <string>
#include <boost/function.hpp>
#include <boost/asio/ssl/context.hpp>

#include <hserv/impl/httpsserverimpl.h>
#include <hserv/serverinterface.h>
#include <hserv/context.h>

namespace hserv {

class ServerImpl;
class Connection;

// The HTTP server implementation class.
class HttpsServer : public ServerInterface
{
public:
    HttpsServer(const std::string &address, int port,
                boost::asio::ssl::context &context,
                const boost::function<void(const boost::shared_ptr<Context> &)> &callback)
        : impl(address, port, context, callback)
    {
    }

    ~HttpsServer() {
    }

    virtual void run() {
        impl.run();
    }

    virtual void stop() {
        impl.stop();
    }

private:
    HttpsServerImpl impl;
};

} // namespace libahttp

#endif // HSERV_HTTPSERVER_H
