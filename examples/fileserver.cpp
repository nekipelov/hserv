/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <fstream>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <hserv/httpserver.h>
#include <hserv/request.h>
#include <hserv/response.h>

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

using namespace hserv;
namespace fs = boost::filesystem;

std::string detectMimeType(const std::string &fileName);
void handler(const boost::shared_ptr<Context> &context);
void handleNextChunk(const boost::shared_ptr<Context> &context, boost::shared_ptr<std::ifstream> in);

int main(int, char**)
{
    int port = 3000;
    std::string address = "0.0.0.0";

    try
    {
        std::cout << "--> Test HTTP Server started on "
                  << "http://" << address << ":" << port
                  << std::endl;

        HttpServer(address, port, handler).run();
    }
    catch (std::exception &e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }

    return 0;
}

void handler(const boost::shared_ptr<Context> &context)
{

    std::cout << context->request().method() << " "
              << context->request().uri() << std::endl;

    std::string fileName;
    const std::string &uri = context->request().uri();

    if( uri.size() < 2 ) { // if url == '/' || url.empty()
        fileName = "index.html";
    }
    else {
        fileName.assign( uri.begin() + 1, uri.end() );

        if( fileName[ fileName.size() - 1 ] == '/'  )
            fileName += "index.html";
        else if( fs::is_directory(fileName) )
            fileName += "/index.html";
    }

    boost::shared_ptr<std::ifstream>
            in(new std::ifstream(fileName.c_str(), std::ios::in | std::ios::binary));

    if( in->good() ) {
        size_t size = fs::file_size(fileName);
        const std::string &s = boost::lexical_cast<std::string>(size);

        context->response().setStatus(Response::Ok);
        context->response().addHeader("Content-Type", detectMimeType(fileName));
        context->response().addHeader("Content-Length", s);

        handleNextChunk(context, in);
    }
    else {
        context->response() = Response::makeResponse(Response::NotFound);
        context->asyncDone();
    }
}


void handleNextChunk(const boost::shared_ptr<Context> &context, boost::shared_ptr<std::ifstream> in)
{
    char buff[4096];

    if( in->read(buff, sizeof(buff)).gcount() > 0 ) {
        context->response().setContent(buff, in->gcount());
        context->asyncPartialSend(boost::bind(handleNextChunk, boost::ref(context), in));
    }
    else {
        context->asyncDone();
    }
}

struct ContentMap {
    const char *ext;
    const char *mime;
    const char *encoding;
    const char *encoding11;
};

ContentMap contentMap[] = {
    { ".html",      "text/html",                    "",             "" },
    { ".htm",       "text/html",                    "",             "" },
    { ".js",        "text/javascript",              "",             "" },
    { ".gif",       "image/gif",                    "",             "" },
    { ".jpeg",      "image/jpeg",                   "",             "" },
    { ".jpg",       "image/jpeg",                   "",             "" },
    { ".jpe",       "image/jpeg",                   "",             "" },
    { ".png",       "image/png",                    "",             "" },
    { ".mp3",       "audio/mpeg",                   "",             "" },
    { ".css",       "text/css",                     "",             "" },
    { ".txt",       "text/plain",                   "",             "" },
    { ".swf",       "application/x-shockwave-flash","",             "" },
    { ".dcr",       "application/x-director",       "",             "" },
    { ".pac",       "application/x-ns-proxy-autoconfig", "",        "" },
    { ".pa",        "application/x-ns-proxy-autoconfig", "",        "" },
    { ".tar",       "multipart/x-tar",              "",             "" },
    { ".gtar",      "multipart/x-gtar",             "",             "" },
    { ".tar.Z",     "multipart/x-tar",              "x-compress",   "compress" },
    { ".tar.gz",    "multipart/x-tar",              "x-gzip",       "gzip" },
    { ".taz",       "multipart/x-tar",              "x-gzip",       "gzip" },
    { ".tgz",       "multipart/x-tar",              "x-gzip",       "gzip" },
    { ".tar.z",     "multipart/x-tar",              "x-pack",       "x-pack" },
    { ".Z",         "application/x-compress",       "x-compress",   "compress" },
    { ".gz",        "application/x-gzip",           "x-gzip",       "gzip" },
    { ".z",         "unknown",                      "x-pack",       "x-pack" },
    { ".bz2",       "application/x-bzip2",          "x-bzip2",      "x-bzip2" },
    { ".ogg",       "application/x-ogg",            "",             "" },
    { ".xbel",      "text/xml",                     "",             "" },
    { ".xml",       "text/xml",                     "",             "" },
    { ".xsl",       "text/xml",                     "",             "" },
    { ".hqx",       "application/mac-binhex40",     "",             "" },
    { ".cpt",       "application/mac-compactpro",   "",             "" },
    { ".doc",       "application/msword",           "",             "" },
    { ".bin",       "application/octet-stream",     "",             "" },
    { ".dms",       "application/octet-stream",     "",             "" },
    { ".lha",       "application/octet-stream",     "",             "" },
    { ".lzh",       "application/octet-stream",     "",             "" },
    { ".exe",       "application/octet-stream",     "",             "" },
    { ".class",     "application/octet-stream",     "",             "" },
    { ".oda",       "application/oda",              "",             "" },
    { ".pdf",       "application/pdf",              "",             "" },
    { ".ai",        "application/postscript",       "",             "" },
    { ".eps",       "application/postscript",       "",             "" },
    { ".ps",        "application/postscript",       "",             "" },
    { ".ppt",       "application/powerpoint",       "",             "" },
    { ".rtf",       "application/rtf",              "",             "" },
    { ".bcpio",     "application/x-bcpio",          "",             "" },
    { ".torrent",   "application/x-bittorrent",     "",             "" },
    { ".vcd",       "application/x-cdlink",         "",             "" },
    { ".cpio",      "application/x-cpio",           "",             "" },
    { ".csh",       "application/x-csh",            "",             "" },
    { ".dir",       "application/x-director",       "",             "" },
    { ".dxr",       "application/x-director",       "",             "" },
    { ".dvi",       "application/x-dvi",            "",             "" },
    { ".hdf",       "application/x-hdf",            "",             "" },
    { ".cgi",       "application/x-httpd-cgi",      "",             "" },
    { ".skp",       "application/x-koan",           "",             "" },
    { ".skd",       "application/x-koan",           "",             "" },
    { ".skt",       "application/x-koan",           "",             "" },
    { ".skm",       "application/x-koan",           "",             "" },
    { ".latex",     "application/x-latex",          "",             "" },
    { ".mif",       "application/x-mif",            "",             "" },
    { ".nc",        "application/x-netcdf",         "",             "" },
    { ".cdf",       "application/x-netcdf",         "",             "" },
    { ".patch",     "application/x-patch",          "",             "" },
    { ".sh",        "application/x-sh",             "",             "" },
    { ".shar",      "application/x-shar",           "",             "" },
    { ".sit",       "application/x-stuffit",        "",             "" },
    { ".sv4cpio",   "application/x-sv4cpio",        "",             "" },
    { ".sv4crc",    "application/x-sv4crc",         "",             "" },
    { ".tar",       "application/x-tar",            "",             "" },
    { ".tcl",       "application/x-tcl",            "",             "" },
    { ".tex",       "application/x-tex",            "",             "" },
    { ".texinfo",   "application/x-texinfo",        "",             "" },
    { ".texi",      "application/x-texinfo",        "",             "" },
    { ".t",         "application/x-troff",          "",             "" },
    { ".tr",        "application/x-troff",          "",             "" },
    { ".roff",      "application/x-troff",          "",             "" },
    { ".man",       "application/x-troff-man",      "",             "" },
    { ".me",        "application/x-troff-me",       "",             "" },
    { ".ms",        "application/x-troff-ms",       "",             "" },
    { ".ustar",     "application/x-ustar",          "",             "" },
    { ".src",       "application/x-wais-source",    "",             "" },
    { ".zip",       "application/zip",              "",             "" },
    { ".au",        "audio/basic",                  "",             "" },
    { ".snd",       "audio/basic",                  "",             "" },
    { ".mpga",      "audio/mpeg",                   "",             "" },
    { ".mp2",       "audio/mpeg",                   "",             "" },
    { ".aif",       "audio/x-aiff",                 "",             "" },
    { ".aiff",      "audio/x-aiff",                 "",             "" },
    { ".aifc",      "audio/x-aiff",                 "",             "" },
    { ".ram",       "audio/x-pn-realaudio",         "",             "" },
    { ".rpm",       "audio/x-pn-realaudio-plugin",  "",             "" },
    { ".ra",        "audio/x-realaudio",            "",             "" },
    { ".wav",       "audio/x-wav",                  "",             "" },
    { ".pdb",       "chemical/x-pdb",               "",             "" },
    { ".xyz",       "chemical/x-pdb",               "",             "" },
    { ".ief",       "image/ief",                    "",             "" },
    { ".tiff",      "image/tiff",                   "",             "" },
    { ".tif",       "image/tiff",                   "",             "" },
    { ".ras",       "image/x-cmu-raster",           "",             "" },
    { ".pnm",       "image/x-portable-anymap",      "",             "" },
    { ".pbm",       "image/x-portable-bitmap",      "",             "" },
    { ".pgm",       "image/x-portable-graymap",     "",             "" },
    { ".ppm",       "image/x-portable-pixmap",      "",             "" },
    { ".rgb",       "image/x-rgb",                  "",             "" },
    { ".xbm",       "image/x-xbitmap",              "",             "" },
    { ".xpm",       "image/x-xpixmap",              "",             "" },
    { ".xwd",       "image/x-xwindowdump",          "",             "" },
    { ".ico",       "image/x-icon",					"",             "" },
    { ".rtx",       "text/richtext",                "",             "" },
    { ".tsv",       "text/tab-separated-values",    "",             "" },
    { ".etx",       "text/x-setext",                "",             "" },
    { ".sgml",      "text/x-sgml",                  "",             "" },
    { ".sgm",       "text/x-sgml",                  "",             "" },
    { ".mpeg",      "video/mpeg",                   "",             "" },
    { ".mpg",       "video/mpeg",                   "",             "" },
    { ".mpe",       "video/mpeg",                   "",             "" },
    { ".qt",        "video/quicktime",              "",             "" },
    { ".mov",       "video/quicktime",              "",             "" },
    { ".avi",       "video/x-msvideo",              "",             "" },
    { ".movie",     "video/x-sgi-movie",            "",             "" },
    { ".ice",       "x-conference/x-cooltalk",      "",             "" },
    { ".wrl",       "x-world/x-vrml",               "",             "" },
    { ".vrml",      "x-world/x-vrml",               "",             "" },
};

std::string detectMimeType(const std::string &fileName)
{
    std::size_t lastSlashPos = fileName.rfind('/');
    std::size_t lastDotPos = fileName.rfind('.');
    std::string extension;

    if (lastDotPos != std::string::npos && (lastDotPos > lastSlashPos || lastSlashPos == std::string::npos) )
    {
        extension = fileName.substr(lastDotPos);

        ContentMap *it = &contentMap[0], *end = &contentMap[sizeof(contentMap) / sizeof(ContentMap)];

        for(; it != end; ++it)
        {
            if( strcasecmp(extension.c_str(), it->ext) == 0 )
                return it->mime;
        }
    }

    static std::string defaultMimeType = "application/octet-stream";
    return defaultMimeType;
}
