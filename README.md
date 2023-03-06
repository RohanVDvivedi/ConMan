# ConnMan
This is a C library, for developing multithreaded networking applications.
It is essentially an abstraction layer over the POSIX sockets library, providing a handler-callback based network agnostic API.
It allows you to change the underlying protocol layers (tcp to udp or ipv4 to ipv6) without any change in application logic.
Additionally, it porvides a stream interface to work with tcp sockets, along with ssl and compression support.

## Setup instructions
**Install dependencies :**
 * [Cutlery](https://github.com/RohanVDvivedi/Cutlery)
 * [BoomPar](https://github.com/RohanVDvivedi/BoomPar)
 * [openssl](https://github.com/openssl/openssl)  ($ sudo apt install libssl-dev)
 * [zlib](https://github.com/madler/zlib)      ($ sudo apt install zlib1g-dev)

**Download source code :**
 * `git clone https://github.com/RohanVDvivedi/ConnMan.git`

**Build from source :**
 * `cd ConnMan`
 * `make clean all`

**Install from the build :**
 * `sudo make install`
 * ***Once you have installed from source, you may discard the build by*** `make clean`

## Using The library
 * add `-lconnman -lboompar -lpthread -lcutlery -lssl -lcrypto -lz` linker flag, while compiling your application
 * do not forget to include appropriate public api headers as and when needed. this includes
   * `#include<server.h>`
   * `#include<client.h>`
   * `#include<client_set.h>`
   * `#include<comm_address.h>`
   * `#include<ssl_ctx_helper.h>`
      Additionally the library also provides the following stream apis, to create appropriate streams as per use.
   * `#include<file_descriptor_stream.h>`
   * `#include<piped_stream.h>`
   * `#include<ssl_stream.h>`
   * `#include<writable_dstring_stream.h>`
   * `#include<zlib_readable_decompression_stream.h>`
   * `#include<zlib_writable_compression_stream.h>`
   * `#include<read_until_dstring_stream.h>`
      Below is the list of headers that contain utility functions that can be used with the created streams.
   * `#include<stream_util.h>`
      Below header container allows you to manage multiple streams each chained with an underlying stream managed as a stack.
   * `#include<stacked_stream.h>`

## Instructions for uninstalling library

**Uninstall :**
 * `cd ConnMan`
 * `sudo make uninstall`
