# attender
A windows/linux RESTful webservice server for professionals built using boost::asio.

1. [Features](https://github.com/5cript/attender#features)
2. [Documentation](https://github.com/5cript/attender#documentation)
3. [Dependencies](https://github.com/5cript/attender#dependencies_and_Requirements)
4. [Build](https://github.com/5cript/attender#how_to_build)
5. [Basics](https://github.com/5cript/attender#basics)
6. [Tutorial](https://github.com/5cript/attender#tutorial)

## Features
### What does attender have:
- High scalability
- SSL/TLS
- rudamentary session support
- cookies
- expressjs like interface

### What does attender not have (yet):
- JSON / XML support (planed)
- keep alive
- custom error code pages.
- HTTP/2

## Documentation
**Doxygen documentation is available in the wiki.**
https://github.com/5cript/attender/wiki

## Dependencies and Requirements
- C++14 compliant compiler (clang or g++ on linux or msys2. MSVC will most likely not compile attender)

- boost asio
- boost filesystem
- openssl

## How to build
This project provides a cmake file (for a static library).
- mkdir build
- cd build
- cmake ..
- make

## Basics
### expressjs inspiration
The library was inspired by express.js. The API is not identical and support is not identical, but if you find this documentation lacking, you can get a first impression at:
- [ExpressJS Request](https://expressjs.com/en/4x/api.html#req)
- [ExpressJS Response](https://expressjs.com/en/4x/api.html#res)

### managed_io_context
The managed io context is a wrapper for boost::asio::io_service. It accepts some kind of attender::async_model which handels the usage of the io_service. You can subclass attender::async_model and provide your own implementation.
You can use io_context/thread_pooler.hpp as an example.

### ssl_context_interface
SSL/TLS servers need a ssl_context. Due to security implications, no guarantees are made for the provided "ssl_example_context" and I highly suggest for you to implement it on your own, if security is highly critical.
The provided implementation shall serve as an example, but is fully functional for server only certificates.

### callbacks
Almost all callbacks to registered routings provide a request_handler and a response_handler. 
These callbacks are called through the io_service run method and do not provide thread safety to the outside of the callback.
The end of any successful request has to be a call to some send function, or to the end function of the response_handler, otherwise there will be no termination of request -> the client will "hang".

### request_handler
The request_handler, abbreviated req in all the examples, is responsible for doing everything on the request. It can read the request header and the content of the request. 

### response_handler
The response_handler, abbreviated res in all the examples, is used to respond to a request. You can respond with just a header or with content from various sources, but only one send instruction can be issued. You cannot for instance send a string followed by stream.
**The most important thing is to end your common handler with a send or end instruction. This is vital, or otherwise the connection will not be closed or deleted!**

### Sessions
Sessions need a session_manager, which in turn requires a session_storage. This library only provides a memory_session_storage,
which keeps all sessions in memory. But this has the drawback, that sessions do not persist the program and cannot be shared between multiple server instances. I recommend you to write a database session storage by subclassing the session_storage_interface. More implementations (mysql, sqlite) might follow in the future in different git repositories.

## Tutorial
### How to create a server
The following code example shows how to create a server and runs it on port 80.
```C++
#include <attender/attender.hpp>

#include <iostream>

int main()
{
    using namespace attender;

    // an io_service wrapper for boost::asio::io_service.
    // you can provide your own implementation, by subclassing "attender::async_model".
    managed_io_context <thread_pooler> context;

    // create a server
    tcp_server server(context.get_io_service(),
        [](auto*, auto const& exc) {
            // some error occured. (this is not thread safe)
            // for now, the first parameter may not be used safely, but represents a connection object.
            std::cerr << exc << "\n"; 
        }
    );

    // start server on port 80. Numbers are also valid
    server.start("http");

    // PAUSE
}
```

### How to create a secure server
```C++
#include <attender/attender.hpp>

#include <openssl/err.h>

#include <iostream>

int main()
{
    using namespace attender;

    // an io_service wrapper for boost::asio::io_service.
    // you can provide your own implementation, by subclassing "attender::async_model".
    managed_io_context <thread_pooler> context;

    // create a server
    tcp_secure_server server(                     
        // boost::asio::io_service
        io_ctx.get_io_service(),
                             
        // An SSL context
        std::unique_ptr <attender::ssl_context_interface> {new ssl_example_context("key.pem", "cert.pem")},
                             
        // An error callback. (here with OpenSSL demangling)
        [](auto* connection, auto const& ec) {
            std::cerr << "error: " << ec << "\n";

            std::string err = ec.message();
            if (ec.category() == boost::asio::error::get_ssl_category()) {
                err =   std::string(" (")
                      + std::to_string(ERR_GET_LIB(ec.value()))+","
                      + std::to_string(ERR_GET_FUNC(ec.value()))+","
                      + std::to_string(ERR_GET_REASON(ec.value()))+") "
                ;
                //ERR_PACK /* crypto/err/err.h */
                char buf[128];
                ::ERR_error_string_n(ec.value(), buf, sizeof(buf));
                err += buf;
            }
            std::cerr << "\t" << err << "\n";
        }
    );

    // start server on port 443. Numbers are also valid
    server.start("https");

    // PAUSE
}
```

### How to add routings
```C++
#include <attender/attender.hpp>

#include <iostream>

int main()
{
    /* Create normal or secure server */
  
    // this will route every GET request for "/test" to this handler
    server.get("/test", [](auto req, auto res) {
         // not threadsafe use of cout
         std::cout << "Someone requested localhost/test\n";
         
         // reply with empty response. (and close connection!)
         res->send_status(204);
    });
    
    // this route shows regex capabilities and path parameters.
    server.get("/\\w*/:param1", [](auto req, auto res) {
        std::cout << req->param("param1") << "\n";
        res->redirect("http://www.google.com:80", 301).end();
    });
    
    server.start(80);
}
```

### How to read
```C++
#include <attender/attender.hpp>

int main() 
{
    /* Create normal or secure server */

    server.post("/read_test", [](auto req, auto res) {
        // The buffer needs to keep alive. For this example a shared pointer is used.
        // But this is not required!
        auto monster = std::make_shared <std::string>();
        req->read_body(*monster).then(
            [monster{monster}, res]()
            {
                std::cout << "all done!\n";
                std::cout << *monster << "\n";
                // end with 204 OK
                res->status(204)->end();
            }
        ).except(
            [extender{monster}](boost::system::error_code ec)
            {
                // something went wrong!
                // res, req got freed!
                std::cout << "except\n";
                
                // do not call res->end or similar here!
                // the connection terminated already!
            }
        );
    });
    
    server.start(80);
```

### How to write
```C++
server.get("/write_test", [](auto req, auto res) {
    res->send("Hello World!");
});
```

### Mounting 
The following example mounts /home/username to the url /mnt.
The callback in this case does not do any response related stuff, but instead is only for checking the request.
The return value of the callback determines whether or not the request shall proceed.
```C++
server.mount("/home/username", "/mnt", [](auto req, auto mres) {
    if (/*...*/) // I do not like this request! STOP THIS AT ONCE! (will return 403)
        return false;
    else // you may proceed
        return true;
// allow for GET, HEAD, OPTIONS and POST methods
}, {mount_options::GET, mount_options::HEAD, mount_options::OPTIONS, mount_options::POST})
```

### Sessions
This is example shows how to get, create and delete a session.
```C++
#include <attender/attender.hpp>

int main()
{
    using namespace attender;
    managed_io_context <thread_pooler> io_ctx;
    
    session_manager sessions {
        std::make_unique <memory_session_storage <uuid_generator, session>>()
    };

    // normal server for simplicity, secure server ofc also possible.
    tcp_server server(io_ctx.get_io_service(),
        [](auto* connection, auto const& ec) {
            std::cerr << "ERROR: " << ec << "\n";
        }
    );
    
    // make new session and terminate old, if there is one.
    server.get("/auth", [&sessions](auto req, auto res) {
        SessionType sess;
        auto state = sessions.load_session ("MYID", sess, req);
        if (state == session_state::live)
            sessions.terminate_session(sess);
        auto newSession = sessions.make_session <SessionType> ();

        cookie ck;
        ck.set_name("MYID").set_value(newSession.id());
        res->set_cookie(ck);
        res->send("ok");
        std::cout << "ok\n";
    });
}
```
