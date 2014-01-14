/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef HSERV_HEADER_H
#define HSERV_HEADER_H

#include <string>

namespace hserv {

struct HeaderItem
{
    std::string name;
    std::string value;
};

} // namespace hserv

#endif // HSERV_HEADER_H
