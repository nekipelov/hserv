/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_HTTPSERVERIMPL_H
#define HSERV_HTTPSERVERIMPL_H

#include <string>
#include <boost/function.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <hserv/impl/httpconnection.h>

namespace hserv {

// The HTTPS server implementation class.
class HttpsServerImpl
{
public:
    explicit HttpsServerImpl(const std::string &address, int port,
                             boost::asio::ssl::context &context,
                             const boost::function<void(const boost::shared_ptr<Context> &)> &callback)
        : listenAddress(address), listenPort(port), ioService(), context(context),
          acceptor(ioService), callback(callback)
    {}

    ~HttpsServerImpl() {}

    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SslSocket;

    struct ShutdownSocket
    {
        void operator()(boost::shared_ptr<SslSocket> &socket)
        {
            boost::system::error_code ignored_ec;
            socket->shutdown(ignored_ec);
            socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        }
    };

    struct CloseSocket
    {
        void operator()(boost::shared_ptr<SslSocket> &socket)
        {
            socket->lowest_layer().close();
        }
    };

    typedef HttpConnection<SslSocket, CloseSocket, ShutdownSocket> ConnectionType;

    void run()
    {
        newSocket.reset( new SslSocket(ioService, context) );
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
        acceptor.async_accept(newSocket->lowest_layer(),
                              boost::bind(&HttpsServerImpl::handleAccept, this,
                                          boost::asio::placeholders::error));

        ioService.run();
    }

    void stop()
    {
        ioService.post(boost::bind(&HttpsServerImpl::handleStop, this));
    }

protected:
    void handleAccept(const boost::system::error_code &ec)
    {
        if ( !ec )
        {
            // Disable Nagle algorithm
            boost::asio::ip::tcp::no_delay option(true);
            newSocket->lowest_layer().set_option(option);

            newSocket->async_handshake(boost::asio::ssl::stream_base::server,
                                       boost::bind(&HttpsServerImpl::handleHandshake, this,
                                                   newConnection, boost::asio::placeholders::error));

            newSocket.reset( new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(ioService, context) );
            newConnection.reset( new ConnectionType(ioService, newSocket, callback));
            acceptor.async_accept(newSocket->lowest_layer(),
                                   boost::bind(&HttpsServerImpl::handleAccept, this,
                                               boost::asio::placeholders::error));
        }
    }

    void handleStop()
    {
        acceptor.close();
        ioService.stop();
    }

    void handleHandshake(const boost::shared_ptr<ConnectionType> &connection,
                         const boost::system::error_code &ec)
    {
        if( !ec )
            connection->start();
    }

private:
    std::string listenAddress;
    int listenPort;

    boost::asio::io_service ioService;
    boost::asio::ssl::context &context;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::function<void(const boost::shared_ptr<Context> &)> callback;
    boost::shared_ptr<ConnectionType> newConnection;
    boost::shared_ptr<SslSocket> newSocket;
};

} // namespace hserv

#endif // HSERV_HTTPSERVERIMPL_H
