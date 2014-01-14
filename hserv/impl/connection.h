/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_CONNECTION_H
#define HSERV_CONNECTION_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

namespace hserv {

class Connection : private boost::noncopyable
{
public:
    virtual ~Connection() {}

    virtual void start() = 0;

    virtual void stop() = 0;

    virtual void writeResponse() = 0;

    virtual void writeResponsePartial(const boost::function<void()> &callback) = 0;

protected:
    virtual void handleComplete(const boost::system::error_code &ec) = 0;
};

} // namespace hserv

#endif // HSERV_CONNECTION_H
