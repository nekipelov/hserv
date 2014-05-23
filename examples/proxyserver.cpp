/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

#include <hserv/httpserver.h>

using namespace hserv;
using namespace boost::asio;

static void usage();
bool verbose = false;

class ProxyPass : public boost::enable_shared_from_this<ProxyPass> {
public:
    ProxyPass(boost::shared_ptr<Context> context, ip::tcp::endpoint endpoint);

    void handle();

    static void startHandler(boost::shared_ptr<Context> context, int port, const std::string &address);
    static void stopHandler();

protected:
    void handleConnect(const boost::system::error_code &ec);
    void handleWriteRequest(const boost::system::error_code &ec);
    void handleWriteRequest09(const boost::system::error_code &ec);
    void handleReadStatusLine(const boost::system::error_code &ec);
    void handleReadHeaders(const boost::system::error_code &ec);
    void handleReadContent(const boost::system::error_code &ec);

private:
    ip::tcp::socket socket;
    ip::tcp::endpoint endpoint;

    streambuf requestBuff;
    streambuf responseBuff;

    size_t contentLength;

    boost::shared_ptr<Context> context;

    static boost::shared_ptr<ProxyPass> newProxyPass;
};

boost::shared_ptr<ProxyPass> ProxyPass::newProxyPass;

ProxyPass::ProxyPass(boost::shared_ptr<Context> context, ip::tcp::endpoint endpoint)
    : socket(context->ioService()), endpoint(endpoint),
      contentLength(std::numeric_limits<size_t>::infinity()),
      context(context)
{
}

void ProxyPass::startHandler(boost::shared_ptr<Context> context, int port, const std::string &address)
{
    ip::address addr = ip::address::from_string(address);
    ip::tcp::endpoint endpoint(addr, port);

    newProxyPass.reset(new ProxyPass(context, endpoint));
    newProxyPass->handle();
}

void ProxyPass::stopHandler()
{
    newProxyPass.reset();
}

void ProxyPass::handle()
{
    socket.async_connect(endpoint, boost::bind(
                      &ProxyPass::handleConnect,
                      shared_from_this(),
                      boost::asio::placeholders::error));
}

void ProxyPass::handleConnect(const boost::system::error_code &ec)
{
    if( !ec )
    {
        std::ostream stream(&requestBuff);

        stream << context->request().method() << " " << context->request().uri();

        if( verbose ) {
            std::cerr << context->request().method() << " "
                      << context->request().uri() << std::endl;
        }

        if( context->request().versionMajor() == 1 ) {
// Для проксирования HTTP/1.1 нужна полная поддержка этого HTTP/1.1
//            if( req.versionMinor() == 1 )
//                stream << " HTTP/1.1\r\n";
//            else
                stream << " HTTP/1.0\r\n";

            const std::vector<HeaderItem> &headers = context->request().headers();
            std::vector<HeaderItem>::const_iterator it = headers.begin(),
                    end = headers.end();

            for(; it != end; ++it) {
                stream << it->name << ": " << it->value << "\r\n";

                if( verbose )
                    std::cerr << it->name << ": " << it->value << std::endl;
            }
        }
        else if( context->request().versionMajor() > 1 )
        {
            std::cerr << "Unsupported HTTP version" << std::endl;
            abort();
        }

        stream << "\r\n";

        if( verbose )
            std::cerr << std::endl;

        const std::vector<char> &postData = context->request().postData();
        std::copy(postData.begin(),
                  postData.end(),
                  std::ostream_iterator<unsigned char>(stream));

        if( context->request().versionMajor() == 1 )
        {
            boost::asio::async_write(socket, requestBuff,
                    boost::bind(&ProxyPass::handleWriteRequest,
                                shared_from_this(),
                                boost::asio::placeholders::error));
        }
        else
        {
            boost::asio::async_write(socket, requestBuff,
                    boost::bind(&ProxyPass::handleWriteRequest09,
                                shared_from_this(),
                                boost::asio::placeholders::error));
        }
    }
    else
    {
        context->response() = Response::makeResponse(Response::BadGateway);
        context->asyncDone();
        std::cerr << "handleConnect: " << ec.message() << std::endl;
    }
}

void ProxyPass::handleWriteRequest(const boost::system::error_code &ec)
{
    if( !ec )
    {
        boost::asio::async_read_until(socket, responseBuff, "\r\n",
                  boost::bind(&ProxyPass::handleReadStatusLine,
                              shared_from_this(),
                              boost::asio::placeholders::error));
    }
    else {
        context->response() = Response::makeResponse(Response::BadGateway);
        context->asyncDone();

        std::cerr << "handleWriteRequest: " << ec.message() << std::endl;
    }
}

void ProxyPass::handleWriteRequest09(const boost::system::error_code &ec)
{
    if( !ec )
    {
        boost::asio::async_read(socket, responseBuff,
                                boost::asio::transfer_at_least(1),
                                boost::bind(&ProxyPass::handleReadContent,
                                            shared_from_this(),
                                            boost::asio::placeholders::error));
    }
    else
    {
        context->response() = Response::makeResponse(Response::BadGateway);
        context->asyncDone();

        std::cerr << "handleWriteRequest09: " << ec.message() << std::endl;
    }
}


void ProxyPass::handleReadStatusLine(const boost::system::error_code &ec)
{
    if( !ec )
    {
        std::istream stream(&responseBuff);
        std::string httpVersion;
        unsigned int statusCode;
        std::string statusMessage;

        stream >> httpVersion;
        stream >> statusCode;

        std::getline(stream, statusMessage);
        if( !stream || httpVersion.substr(0, 5) != "HTTP/" )
        {
          std::cerr << "Invalid response" << std::endl;
          return;
        }

        if( verbose )
            std::cerr << httpVersion << " " << statusCode << " " << statusMessage << std::endl << std::endl;

        context->response().setStatus( static_cast<Response::StatusType>(statusCode) );

        boost::asio::async_read_until(socket, responseBuff, "\r\n\r\n",
            boost::bind(&ProxyPass::handleReadHeaders,
                        shared_from_this(),
                        boost::asio::placeholders::error));
    }
    else {
        context->response() = Response::makeResponse(Response::BadGateway);
        context->asyncDone();

        std::cerr << "handleReadStatusLine: " << ec.message() << std::endl;
    }
}

void ProxyPass::handleReadHeaders(const boost::system::error_code &ec)
{
    if( !ec )
    {
         std::istream stream(&responseBuff);
         std::string header;

         while( std::getline(stream, header) && header != "\r" )
         {
             std::string::size_type colon = header.find_first_of(':');

             if( colon == std::string::npos ) {
                 std::cerr << "Invalid header " << header << "\n";
                 return;
             }

             std::string part1 = header.substr(0, colon);
             std::string part2 = header.substr(colon + 2, header.size() - colon - 3);

             if( strcasecmp(part1.c_str(), "content-length") == 0 )
                 contentLength = boost::lexical_cast<size_t>( part2 );

             context->response().addHeader(part1, part2);
         }

         if (responseBuff.size() > 0) {
             const char *p = boost::asio::buffer_cast<const char*>(responseBuff.data());
             context->response().addContent(p, responseBuff.size());
             responseBuff.consume(responseBuff.size());

             if( contentLength != std::numeric_limits<size_t>::infinity()
                 && contentLength == context->response().content().size() ) {
                     context->asyncDone();
                     return;
             }
         }

         switch(context->response().status()) {
         case Response::NoContent:   // must not include body
         case Response::NotModified: // must not include body
             context->asyncDone();
             break;
         default:
             boost::asio::async_read(socket, responseBuff,
                 boost::asio::transfer_at_least(1),
                 boost::bind(&ProxyPass::handleReadContent,
                             shared_from_this(),
                             boost::asio::placeholders::error));
             break;
         }
    }
    else {
        context->response() = Response::makeResponse(Response::BadGateway);
        context->asyncDone();

        std::cerr << "handleReadHeaders: " << ec.message() << std::endl;
    }
}

void ProxyPass::handleReadContent(const boost::system::error_code &ec)
{
    if( !ec )
    {
        const char *p = boost::asio::buffer_cast<const char*>(responseBuff.data());
        context->response().addContent(p, responseBuff.size());
        responseBuff.consume(responseBuff.size());

        if( contentLength != std::numeric_limits<size_t>::infinity()
                && contentLength == context->response().content().size() ) {
            context->asyncDone();
        }
        else {
            boost::asio::async_read(socket, responseBuff,
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&ProxyPass::handleReadContent,
                                                shared_from_this(),
                                                boost::asio::placeholders::error));
        }
    }
    else if (ec != boost::asio::error::eof)
    {
        context->response() = Response::makeResponse(Response::BadGateway);
        context->asyncDone();

        std::cerr << "handleReadContent: " << ec.message() << "\n";
    }
    else
    {
        context->asyncDone();
    }
}

int main(int argc, char** argv)
{
    int listenPort = 3000;
    std::string listenAddress = "0.0.0.0";

    int passPort = 80;
    std::string passAddress = "127.0.0.1";

    for(int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if( strcmp(arg, "--help") == 0 )
        {
            usage();
            exit( EXIT_SUCCESS );
        }
        else if( strcmp(arg, "--verbose") == 0 )
        {
            verbose = true;
            ++i;
        }
        else if( strcmp(arg, "--port") == 0 )
        {
            if( i + 1 < argc )
            {
                listenPort = atoi(argv[++i]);
            }
            else
            {
                usage();
                exit( EXIT_FAILURE );
            }
        }
        else if( strcmp(arg, "--address") == 0 )
        {
            if( i + 1 < argc )
            {
                listenAddress = argv[++i];
            }
            else
            {
                usage();
                exit( EXIT_FAILURE );
            }
        }
        else if( strcmp(arg, "--server-port") == 0 )
        {
            if( i + 1< argc )
            {
                passPort = atoi(argv[++i]);
            }
            else
            {
                usage();
                exit( EXIT_FAILURE );
            }
        }
        else if( strcmp(arg, "--server-address") == 0 )
        {
            if( i + 1 < argc )
            {
                passAddress = argv[++i];
            }
            else
            {
                usage();
                exit( EXIT_FAILURE );
            }
        }
        else
        {
            usage();
            exit( EXIT_FAILURE );
        }
    }

    try
    {
        std::cout << "--> Proxy server started on "
                  << "http://" << listenAddress << ":" << listenPort
                  << std::endl;

        boost::asio::io_service ioService;
        HttpServer server(ioService, listenAddress, listenPort,
                          boost::bind(ProxyPass::startHandler, _1, passPort, passAddress));
        server.run();
        ioService.run();

        ProxyPass::stopHandler();
    }
    catch (const std::exception &e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }

    return 0;
}

void usage()
{
    std::cerr << "Usage: proxyserver [OPTION] ... \n";
    std::cerr << "    Options:\n";
    std::cerr << "        --verbose            more details about request\n";
    std::cerr << "        --port <port>        bind to port, default 3000\n";
    std::cerr << "        --address <address>  bind to address, default 0.0.0.0\n";
    std::cerr << "        --server-port <port>  http server port, default 80\n";
    std::cerr << "        --server-address <address>  http server address, default 127.0.0.1\n";
}
