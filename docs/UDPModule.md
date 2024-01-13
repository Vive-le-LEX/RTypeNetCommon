# UDP Module

## Introduction

The UDP Module is a module that allows you to create servers and clients using the UDP protocol.

## Creating a UDP server

First of all you need to create your server Class whose parent is RType::net::UdpServerInterface.

The UdpServerInterface class has a constructor that takes as parameters an asio::io_context and a port. So you need to create an asio::io_context and pass it to the constructor. And don't forget to call the run method of the asio::io_context.

```cpp
#include "NetUdpServer.hpp"

class MyUdpServer : public RType::net::UdpServerInterface {
public:
    MyUdpServer(asio::io_context& context, uint16_t port) : RType::net::UdpServerInterface(context, port) {}
};
```

In your main function you need to create an asio::io_context and pass it to the server constructor. It is preferable to run the asio::io_context in a thread.

```cpp
#include "NetUdpServer.hpp"
#include <thread>

int main() {
    asio::io_context context;
    MyUdpServer server(context, 4242);

    std::thread t([&context]() { context.run(); });

    server->Start();

    t.join();
    return 0;
}
```

Then you need to implement the following methods:

```cpp
void onStarted();
void onStopped();
void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size);
void onSent(const asio::ip::udp::endpoint& endpoint, size_t sent);
void onError(int error, const std::string& category, const std::string& message);
```

The onStarted method is called when the server is started.  
The onStopped method is called when the server is stopped.  
The onReceived method is called when a message is received from a client.  
The onSent method is called when a message is sent to a client.  
The onError method is called when an error occurs.  

```cpp
void onStarted() {
    std::cout << "Server started" << std::endl;
    this->ReceiveAsync();
}

void onStopped() {
    std::cout << "Server stopped" << std::endl;
}

void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) {
    // Add your code here
    this->ReceiveAsync();
}

void onSent(const asio::ip::udp::endpoint& endpoint, size_t sent) {
    // Add your code here
}

void onError(int error, const std::string& category, const std::string& message) {
        std::cout << "error: " << error << " in " << category << ": " << message << std::endl;
}
```

## Creating a UDP client

Contrary to the TCP client, the UDP client is almost the same as the UDP server. The only difference is that the UDP client doesn't have the onStarted and onStopped methods. But instead it has the onConnected and onDisconnected methods.

```cpp
#include "NetUdpClient.hpp"

class MyUdpClient : public RType::net::UdpClientInterface {
public:
    MyUdpClient(asio::io_context& context, const std::string& host, const uint16_t port) : RType::net::UdpClientInterface(context, host, port) {}
};
```

In your main function you need to create an asio::io_context and pass it to the client constructor. It is preferable to run the asio::io_context in a thread.

```cpp
#include "NetUdpClient.hpp"
#include <thread>

int main() {
    asio::io_context context;
    MyUdpClient client(context, "IP", 4242);

    std::thread t([&context]() { context.run(); });

    client->Start();

    t.join();

    return 0;
}
```

Then you need to implement the following methods:

```cpp
void onConnected();
void onDisconnected();
void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size);
void onSent(const asio::ip::udp::endpoint& endpoint, size_t sent);
void onError(int error, const std::string& category, const std::string& message);
```

The onConnected method is called when the client is connected to the server.  
The onDisconnected method is called when the client is disconnected from the server.  
The onReceived method is called when a message is received from the server.  
The onSent method is called when a message is sent to the server.  
The onError method is called when an error occurs.  

```cpp
void onConnected() {
    std::cout << "Client connected" << std::endl;
}

void onDisconnected() {
    std::cout << "Client disconnected" << std::endl;
}

void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) {
    // Add your code here
    this->ReceiveAsync();
}

void onSent(const asio::ip::udp::endpoint& endpoint, size_t sent) {
    // Add your code here
}

void onError(int error, const std::string& category, const std::string& message) {
    std::cout << "error: " << error << " in " << category << ": " << message << std::endl;
}
```

## Sending a message

To send a message you need to call the Send method of the server or the client.
You can send a message to a specific endpoint or to all clients listening to the server.

```cpp
void Send(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size);
void Send(const void* buffer, size_t size);
```

The first method will send the message to the specified endpoint.
The second method will send the message to all clients listening to the server.

```cpp
// Send a message to a specific endpoint
asio::ip::udp::endpoint endpoint(asio::ip::address::from_string("IP"), 4242);
std::string message = "Hello World";
server->Send(endpoint, message.c_str(), message.size());

// Send a message to all clients listening to the server
std::string message = "Hello World";
server->Send(message.c_str(), message.size());
```

## Receiving a message

When a message is received from a client, the onReceived method is called.
It is up to you to implement the onReceived method.

```cpp
void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) {
    // Add your code here
    this->ReceiveAsync();
}
```

<div class="section_buttons">
| Previous          |                              Next |
|:------------------|----------------------------------:|
| [TCP Tutorial](TCPModule.md) | []() |
</div>
