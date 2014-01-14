/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_SERVER_H
#define HSERV_SERVER_H

#include <boost/noncopyable.hpp>

namespace hserv {

class Connection;

// Server interface class
class ServerInterface : private boost::noncopyable
{
public:
    virtual ~ServerInterface() {};

    /// Run the server's io_service loop.
    virtual void run() = 0;

    /// Stop the server.
    virtual void stop() = 0;
};

} // namespace hserv

#endif // HSERV_SERVER_H
