/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_FASTCGISERVER_H
#define HSERV_FASTCGISERVER_H

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

#include <hserv/context.h>
#include <hserv/serverinterface.h>
#include <hserv/impl/fastcgiserverimpl.h>

namespace hserv {

// The FastCGI server implementation class.
class FastCgiServer : public ServerInterface
{
public:
    FastCgiServer(const boost::function<void(const boost::shared_ptr<Context> &)> &callback) {
        pimpl.reset(new FastCgiStdioServerImpl(callback));
    }

    FastCgiServer(const std::string &unixSocket,
                  const boost::function<void(const boost::shared_ptr<Context> &)> &callback) {
        ::unlink(unixSocket.c_str());
        pimpl.reset(new FastCgiUnixSocketServerImpl(unixSocket, callback));
    }

    FastCgiServer(const std::string &address, int port,
                  const boost::function<void(const boost::shared_ptr<Context> &)> &callback) {
        pimpl.reset(new FastCgiNetworkServerImpl(address, port, callback));

    }

    ~FastCgiServer() {};

    virtual void run() {
        pimpl->run();
    }

    virtual void stop() {
        pimpl->stop();
    }

private:
    boost::scoped_ptr<FastCgiServerImpl> pimpl;
};

} // namespace hserv

#endif // HSERV_FASTCGISERVER_H
