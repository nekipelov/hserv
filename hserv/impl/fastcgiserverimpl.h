/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <boost/shared_ptr.hpp>
#include <hserv/impl/fastcgiconnection.h>

namespace hserv {

class FastCgiServerImpl {
public:
    virtual void run() = 0;
    virtual void stop() = 0;
};

class FastCgiNetworkServerImpl : public FastCgiServerImpl {
public:
    typedef FastCGIConnection<boost::asio::ip::tcp::socket> ConnectionType;

    FastCgiNetworkServerImpl(const std::string &address, int port,
                             const boost::function<void(const boost::shared_ptr<Context> &)> &callback)
        : listenAddress(address), listenPort(port), ioService(), acceptor(ioService),
          callback(callback)
    {
    }

    void run()
    {
        newConnection.reset(new ConnectionType(ioService, callback));

        boost::asio::ip::tcp::resolver resolver(ioService);
        boost::asio::ip::tcp::resolver::query query(listenAddress, std::string() );
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

        endpoint.port(listenPort);
        acceptor.open(endpoint.protocol());
#ifndef WIN32
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
#endif
        acceptor.bind(endpoint);
        acceptor.listen();
        acceptor.async_accept(newConnection->getSocket(),
                              boost::bind(&FastCgiNetworkServerImpl::handleAccept, this,
                                          boost::asio::placeholders::error));

        ioService.run();
    }

    void stop()
    {
        ioService.post(boost::bind(&FastCgiNetworkServerImpl::handleStop, this));
    }

protected:
    void handleAccept(const boost::system::error_code &ec)
    {
        if( !ec )
        {
            newConnection->start();
            newConnection.reset(new ConnectionType(ioService, callback));
            acceptor.async_accept(newConnection->getSocket(),
                                   boost::bind(&FastCgiNetworkServerImpl::handleAccept, this,
                                               boost::asio::placeholders::error));
        }
    }

    void handleStop()
    {
        acceptor.close();
        ioService.stop();
    }

private:
    std::string listenAddress;
    int listenPort;

    // The io_service used to perform asynchronous operations.
    boost::asio::io_service ioService;

    // Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor acceptor;

    // The handler for all incoming requests.
    boost::function<void(const boost::shared_ptr<Context> &)> callback;

    // The next connection to be accepted.
    boost::shared_ptr<ConnectionType> newConnection;
};

class FastCgiStdioServerImpl : public FastCgiServerImpl {
public:
    typedef FastCGIConnection<boost::asio::local::stream_protocol::socket> ConnectionType;

    FastCgiStdioServerImpl(const boost::function<void(const boost::shared_ptr<Context> &)> &callback)
        : ioService(), acceptor(ioService), callback(callback)
    {
    }

    void run()
    {
        newConnection.reset(new ConnectionType(ioService, callback));

        acceptor.assign(boost::asio::local::stream_protocol(), 0);
        acceptor.listen();
        acceptor.async_accept(newConnection->getSocket(),
                              boost::bind(&FastCgiStdioServerImpl::handleAccept, this,
                                          boost::asio::placeholders::error));

        ioService.run();

    }

    void stop()
    {
        ioService.post(boost::bind(&FastCgiStdioServerImpl::handleStop, this));
    }

protected:
    void handleAccept(const boost::system::error_code &ec)
    {
        if( !ec )
        {
            newConnection->start();
            newConnection.reset(new ConnectionType(ioService, callback));
            acceptor.async_accept(newConnection->getSocket(),
                                   boost::bind(&FastCgiStdioServerImpl::handleAccept, this,
                                               boost::asio::placeholders::error));
        }
    }

    void handleStop()
    {
        acceptor.close();
        ioService.stop();
    }

private:
    boost::asio::io_service ioService;
    boost::asio::local::stream_protocol::acceptor acceptor;
    boost::function<void(const boost::shared_ptr<Context> &)> callback;
    boost::shared_ptr<ConnectionType> newConnection;
};

class FastCgiUnixSocketServerImpl : public FastCgiServerImpl
{
public:
    typedef FastCGIConnection<boost::asio::local::stream_protocol::socket> ConnectionType;

    FastCgiUnixSocketServerImpl(const std::string &endpoint,
                                const boost::function<void(const boost::shared_ptr<Context> &)> &callback)
        : ioService(), acceptor(ioService, boost::asio::local::stream_protocol::endpoint(endpoint)), callback(callback)
    {
    }

    void run()
    {
        newConnection.reset(new ConnectionType(ioService, callback));

        ::unlink( endpoint.c_str() );
        boost::asio::local::datagram_protocol::endpoint ep(endpoint);
        acceptor.async_accept(newConnection->getSocket(),
                              boost::bind(&FastCgiUnixSocketServerImpl::handleAccept, this,
                                          boost::asio::placeholders::error));

        ioService.run();

    }

    void stop()
    {
        ioService.post(boost::bind(&FastCgiUnixSocketServerImpl::handleStop, this));
    }

protected:
    void handleAccept(const boost::system::error_code &ec)
    {
        if( !ec )
        {
            newConnection->start();
            newConnection.reset(new ConnectionType(ioService, callback));
            acceptor.async_accept(newConnection->getSocket(),
                                   boost::bind(&FastCgiUnixSocketServerImpl::handleAccept, this,
                                               boost::asio::placeholders::error));
        }
    }

    void handleStop()
    {
        acceptor.close();
        ioService.stop();
    }

private:
    boost::asio::io_service ioService;
    boost::asio::local::stream_protocol::acceptor acceptor;
    std::string endpoint;
    boost::function<void(const boost::shared_ptr<Context> &)> callback;
    boost::shared_ptr<ConnectionType> newConnection;
};

} // namespace hserv
