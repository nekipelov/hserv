# hserv

Build status: [![Build Status](https://travis-ci.org/nekipelov/hserv.svg?branch=master)](https://travis-ci.org/nekipelov/hserv)

Is a library for asynchronous http applications based on Boost.Asio. Currently supported:

* http (0.9, 1.0, 1.1)
* https
* fastcgi (stdin/stdout, unix sockets, tcp/ip sockets)

## Quick start

Simple C++11 application:

    #include <libahttp/httpserver.h>
    #include <libahttp/context.h>
    
    using namespace libahttp;
    
    int main(int, char**)
    {
        auto handler = [](boost::shared_ptr<Context> context) {
            auto &response = context->response();
            response.setStatus( Response::Ok );
            response.addHeader("Content-type", "text/html" );
            response.setContent("<h1>Hello world!</h1>");
            context->asyncDone();
        };
    
        HttpServer("0.0.0.0", 3000, handler).run();
    
        return 0;
    }

## TODO

* SCGI
* SPDY
* Documentation
