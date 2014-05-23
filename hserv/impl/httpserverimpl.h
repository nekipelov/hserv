/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_HTTPSERVERIMPL_H
#define HSERV_HTTPSERVERIMPL_H

#include <string>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>


#include <hserv/impl/httpconnection.h>

namespace hserv {

// The HTTP server implementation class.
class HttpServerImpl
{
public:
    HttpServerImpl(boost::asio::io_service &ioService,
                   const std::string &address, int port,
                   const boost::function<void(const boost::shared_ptr<Context> &)> &callback)
        : listenAddress(address), listenPort(port),
          ioService(ioService), acceptor(ioService), callback(callback)
    {}

    ~HttpServerImpl() {}

    typedef boost::asio::ip::tcp::socket TcpSocket;

    struct ShutdownSocket
    {
        void operator()(boost::shared_ptr<TcpSocket> &socket)
        {
            boost::system::error_code ignored_ec;
            socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        }
    };

    struct CloseSocket
    {
        void operator()(boost::shared_ptr<TcpSocket> &socket)
        {
            socket->close();
        }
    };

    typedef HttpConnection<TcpSocket, CloseSocket, ShutdownSocket> ConnectionType;

    void run()
    {
        newSocket.reset( new boost::asio::ip::tcp::socket(ioService) );
        newConnection.reset( new ConnectionType(ioService, newSocket, callback));

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
        acceptor.async_accept(*newSocket.get(),
                              boost::bind(&HttpServerImpl::handleAccept, this,
                                          boost::asio::placeholders::error));
    }

    void stop()
    {
        ioService.post(boost::bind(&HttpServerImpl::handleStop, this));
    }

protected:
    void handleAccept(const boost::system::error_code &ec)
    {
        if( !ec )
        {
            // Disable Nagle algorithm
            boost::asio::ip::tcp::no_delay option(true);
            newSocket->set_option(option);

            // start work
            newConnection->start();

            // prepare next request
            newSocket.reset( new boost::asio::ip::tcp::socket(ioService) );
            newConnection.reset( new ConnectionType(ioService, newSocket, callback));

            acceptor.async_accept(*newSocket.get(),
                                   boost::bind(&HttpServerImpl::handleAccept, this,
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

    boost::asio::io_service &ioService;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::function<void(const boost::shared_ptr<Context> &)> callback;
    boost::shared_ptr<ConnectionType> newConnection;
    boost::shared_ptr<TcpSocket> newSocket;

};

} // namespace hserv

#endif // HSERV_HTTPSERVERIMPL_H
