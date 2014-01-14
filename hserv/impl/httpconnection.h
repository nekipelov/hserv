/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_HTTPCONNECTION_H
#define HSERV_HTTPCONNECTION_H

#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/mpl/bool.hpp>

#include <hserv/response.h>
#include <hserv/request.h>
#include <hserv/impl/requestparser.h>
#include <hserv/impl/connection.h>
#include <hserv/context.h>

namespace hserv {

template<typename T, typename CloseSocket, typename ShutdownSocket>
class HttpConnection: public boost::enable_shared_from_this< HttpConnection<T,CloseSocket,ShutdownSocket> >,
        public Connection
{
public:
    typedef T Socket;

    HttpConnection(boost::asio::io_service &io_service, const boost::shared_ptr<T> &socket,
                   boost::function<void(const boost::shared_ptr<Context> &)> callback)
        : Connection(), io_service(io_service), firstChunk(true), closeConnection(false),
          socket(socket), callback(callback), request(requestImpl)
    {
    }

    ~HttpConnection()
    {
    }

    virtual void start()
    {
        try
        {
            requestImpl = RequestImpl();
            // FIXME - SSL
            // requestImpl.endpoint = socket->remote_endpoint();
            response = Response();
            context.reset( new Context(io_service, *this, request, response) );
            firstChunk = true;

            socket->async_read_some(boost::asio::buffer(buffer),
                                    boost::bind(&HttpConnection::handleRead,
                                                this->shared_from_this(),
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        }
        catch(std::exception &e)
        {
            stop();
        }
    }

    virtual void stop()
    {
        CloseSocket fnc;
        fnc(socket);
    }

    virtual void writeResponse()
    {
        typedef std::vector<boost::asio::const_buffer> Buffers;
        Buffers buffers;
        static const char crlf[] = {'\r', '\n'};

        if( firstChunk == true && requestImpl.versionMajor == 1 ) {
            Buffers status = response.statusBuffers(requestImpl.versionMajor,
                                                    requestImpl.versionMinor);

            if( requestImpl.versionMajor == 1 && requestImpl.versionMinor == 0 &&
                    requestImpl.keepAlive )
            {
                std::string connection = response.header("Connection");

                if( connection.empty() )
                    response.addHeader("Connection", "Keep-Alive");
                else
                    closeConnection = strcasecmp(connection.c_str(), "close") == 0;
            }

            Buffers headers = response.headerBuffers();

            buffers.reserve( status.size() + headers.size() );
            buffers.assign( status.begin(), status.end() );
            buffers.insert( buffers.end(), headers.begin(), headers.end() );
            buffers.push_back( boost::asio::buffer(crlf) );

            firstChunk = false;
        }

        const std::vector<char> &content = response.content();

        buffers.push_back( boost::asio::buffer(content) );

        boost::asio::async_write(*socket.get(),
                                 buffers,
                                 boost::bind(
                                     &HttpConnection::handleComplete,
                                     this->shared_from_this(),
                                     boost::asio::placeholders::error) );
    }

    virtual void writeResponsePartial(const boost::function<void()> &callback)
    {
        typedef std::vector<boost::asio::const_buffer> Buffers;
        Buffers buffers;
        static const char crlf[] = {'\r', '\n'};

        if( firstChunk == true && requestImpl.versionMajor == 1 ) {
            Buffers status = response.statusBuffers(requestImpl.versionMajor,
                                                    requestImpl.versionMinor);

            if( requestImpl.versionMajor == 1 && requestImpl.versionMinor == 0 &&
                    requestImpl.keepAlive )
            {
                std::string connection = response.header("Connection");

                if( connection.empty() )
                    response.addHeader("Connection", "Keep-Alive");
                else
                    closeConnection = strcasecmp(connection.c_str(), "close") == 0;
            }

            Buffers headers = response.headerBuffers();

            buffers.reserve( status.size() + headers.size() );
            buffers.assign( status.begin(), status.end() );
            buffers.insert( buffers.end(), headers.begin(), headers.end() );
            buffers.push_back( boost::asio::buffer(crlf) );

            firstChunk = false;
        }

        const std::vector<char> &content = response.content();

        buffers.push_back( boost::asio::buffer(content) );



        boost::asio::async_write(*socket.get(),
                                 buffers,
                                 boost::bind(
                                     &HttpConnection::handlePartialComplete,
                                     this->shared_from_this(),
                                     callback,
                                     boost::asio::placeholders::error) );
    }

protected:

    // Handle completion of a write operation.
    void handleComplete(const boost::system::error_code &ec)
    {
        if( !ec )
        {
            if( closeConnection == false && requestImpl.keepAlive == true )
            {
                requestParser = RequestParser();
                start();

                return;
            }
            else
            {
                // Initiate graceful connection closure.
                ShutdownSocket fnc;
                fnc(socket);
            }
        }
        else if( ec != boost::asio::error::operation_aborted )
        {
            stop();
        }
    }

    void handlePartialComplete(const boost::function<void()> &callback,
                               const boost::system::error_code &ec)
    {
        if( !ec )
        {
            response.setContent(std::vector<char>());

            if( callback )
                callback();
        }
        else if( ec != boost::asio::error::operation_aborted )
        {
            stop();
        }
    }

    // Handle completion of a read operation.
    void handleRead(const boost::system::error_code &ec,
                    std::size_t bytes_transferred)
    {
        if( !ec )
        {
            RequestParser::ParseState state = requestParser.parse(
                        requestImpl, buffer.data(), buffer.data() + bytes_transferred);

            if( state ==  RequestParser::CompletedState )
            {
                assert( callback.empty() == false );
                callback(context);
            }
            else if( state ==  RequestParser::ErrorState )
            {
                response = Response::makeResponse(Response::BadRequest);
                writeResponse();
            }
            else
            {
                // Incompleted
                socket->async_read_some(boost::asio::buffer(buffer),
                                        boost::bind(&HttpConnection::handleRead,
                                            this->shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
            }
        }
        else if( ec != boost::asio::error::operation_aborted )
        {
            stop();
        }
    }

    boost::asio::io_service &io_service;

    // The parser for the incoming request.
    RequestParser requestParser;

    bool firstChunk;
    bool closeConnection;

    // Socket for the connection.
    boost::shared_ptr<Socket> socket;

    // The handler used to process the incoming request.
    boost::function<void(const boost::shared_ptr<Context> &)> callback;

    // Buffer for incoming data.
    boost::array<char, 8192> buffer;

    // The incoming request.
    RequestImpl requestImpl;
    Request request;

    // The response to be sent back to the client.
    Response response;

    boost::shared_ptr<Context> context;
};

} // namespace hserv

#endif // HSERV_HTTPCONNECTION_H
