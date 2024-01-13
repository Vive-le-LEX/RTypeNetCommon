# TCP Module

## Introduction

The TCP Module is a module that allows you to create servers and clients using the TCP protocol.

## Creating a server

First of all you need to create your server Class whose parent is RTYpe::net::ServerInterface.
But before that you need to create the MessageType enum that will be used to identify the messages you will send and receive.

```cpp
#include "NetServer.hpp"

enum class MessageType : uint8_t {
    Message1,
    Message2,
    Message3
};

template<typename MessageType>
class MyServer : public RType::net::ServerInterface<MessageType> {
public:
    MyServer(uint16_t port) : RType::net::ServerInterface<MessageType>(port) {}
};
```

Or you can use the provided Singleton class to use the server as a singleton.

```cpp
#include "NetServer.hpp"
#include "Singleton.hpp"

enum class MessageType : uint8_t {
    Message1,
    Message2,
    Message3
};

class MyServer : public RType::net::ServerInterface<MessageType>, public RType::Singleton<MyServer> {
private:
    MyServer(uint16_t port) : RType::net::ServerInterface<MessageType>(port) {}
    friend class RType::Singleton<MyServer>;
};
```
Then you need to implement the following methods:

```cpp
bool OnClientConnect(std::shared_ptr<RType::net::TcpConnection<MessageType>> client);
void OnClientDisconnect(std::shared_ptr<RType::net::TcpConnection<MessageType>> client);
void OnMessage(std::shared_ptr<RType::net::TcpConnection<MessageType>> client, RType::net::Message<MessageType>& msg);
void OnClientValidated(std::shared_ptr<RType::net::TcpConnection<MessageType>> client);
```

The OnClientConnect method is called when a client connects to the server.  
The OnClientDisconnect method is called when a client disconnects from the server.  
The OnMessage method is called when a message is received from a client.  
The OnClientValidated method is called when a client is validated by the server.  

```cpp
bool OnClientConnect(std::shared_ptr<RType::net::TcpConnection<MessageType>> client) {
    std::cout << "Client connected with id: " << client->GetId() << std::endl;
    return true;
}

void OnClientDisconnect(std::shared_ptr<RType::net::TcpConnection<MessageType>> client) {
    std::cout << "Client disconnected with id: " << client->GetId() << std::endl;
}

void OnMessage(std::shared_ptr<RType::net::TcpConnection<MessageType>> client, RType::net::Message<MessageType>& msg) {
    std::cout << "Message received from client with id: " << client->GetId() << std::endl;
    switch (msg.header.id) {
        case MessageType::Message1:
            std::cout << "Message1 received" << std::endl;
            break;
        case MessageType::Message2:
            std::cout << "Message2 received" << std::endl;
            break;
        case MessageType::Message3:
            std::cout << "Message3 received" << std::endl;
            break;
        default:
            std::cout << "Unknown message received" << std::endl;
            break;
    }
}

void OnClientValidated(std::shared_ptr<RType::net::TcpConnection<MessageType>> client) {
    std::cout << "Client validated with id: " << client->GetId() << std::endl;
}
```

For the OnMessage method, instead of a switch you can add an unordered_map that will map the message id to a function that will be called when the message is received.

```cpp
void OnMessage(std::shared_ptr<RType::net::TcpConnection<MessageType>> client, RType::net::Message<MessageType>& msg) {
    std::cout << "Message received from client with id: " << client->GetId() << std::endl;
    if (handlers_.find(msg.header.id) != handlers_.end()) {
        handlers_[msg.header.id](client, msg);
    }
}

// Add this to the private section of the class
using handlerClientType = std::shared_ptr<net::TcpConnection<RType::ServerMessages>>&;
using handlerType = std::function<void(handlerClientType&, net::message<RType::ServerMessages>&)>;
std::unordered_map<ServerMessages, handlerType> handlers_;
```
And then you can add the handlers like this in the constructor of the server class.

```cpp
handlers_[MessageType::Message1] = [this](auto&& PH1, auto&& PH2) { return yourFunctionToHandleMessage1(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); };
```


Then you need to call the Start method to start the server.

```cpp
MyServer<MessageType> server(8080);
server.Start();
```

If you want to use the server as a singleton, you first need to call the Construct method to initialize the server.

```cpp
MyServer<MessageType>::Construct(8080);
```

Then you can get the instance of the server by calling the GetInstance method.

```cpp
MyServer<MessageType>::GetInstance()->Start();
```

### Polling messages

Just simply call the Update method to poll messages.

```cpp
server.Update();
```

The Update method takes 2 parameters, the first one is the maximum number of messages to poll and the second one is if it should block the thread if there are no messages to poll.
If you want to poll all the messages without blocking the thread, you can do it like this:

```cpp
server.Update(-1, false);
```

### Sending messages

To send a message you need to create a message, then choose the client you want to send the message to and then call the Send method.

```cpp
RType::net::Message<MessageType> msg;
msg.header.id = MessageType::Message1;
server.MessageClient(client, msg);
```

Or you can send a message to all the clients by calling the MessageAllClients method.

```cpp
RType::net::Message<MessageType> msg;
msg.header.id = MessageType::Message1;
server.MessageAllClients(msg);
```

Or you can send a message to all the clients except one by calling the MessageAllClients method but with the client you don't want to send the message to as a parameter.

```cpp
RType::net::Message<MessageType> msg;
msg.header.id = MessageType::Message1;
server.MessageAllClients(msg, client);
```

## Creating a client

First of all you need to create your client Class whose parent is RType::net::ClientInterface.
But before that you need to create the MessageType enum that will be used to identify the messages you will send and receive.

```cpp
#include "NetClient.hpp"

// This is the enum that will be used to identify the messages, it must be the same as the one used by the server
enum class MessageType : uint8_t {
    Message1,
    Message2,
    Message3
};

template<typename MessageType>
class MyClient : public RType::net::ClientInterface<MessageType> {
    public:
    MyClient() : RType::net::ClientInterface<MessageType>() {}
};
```

Or you can use the provided Singleton class to use the client as a singleton.

```cpp
#include "NetClient.hpp"
#include "Singleton.hpp"

// This is the enum that will be used to identify the messages, it must be the same as the one used by the server
enum class MessageType : uint8_t {
    Message1,
    Message2,
    Message3
};

template<typename MessageType>
class MyClient : public RType::net::ClientInterface<MessageType>, public RType::Singleton<MyClient<MessageType>> {
    private:
    MyClient() : RType::net::ClientInterface<MessageType>() {}
    friend class RType::Singleton<MyClient<MessageType>>;
};
```

Then you need to call the Connect method to connect the client to the server.

```cpp
MyClient<MessageType> client;
client.ConnectToServer("IP", 8080);
```

If you want to use the client as a singleton, you first need to call the Construct method to initialize the client.

```cpp
MyClient<MessageType>::Construct();
```

Then you can get the instance of the client by calling the GetInstance method.

```cpp
MyClient<MessageType>::GetInstance()->ConnectToServer("IP", 8080);
```

### Polling messages

To poll messages you first need to check if the client is connected to the server and then check if there are messages to poll.

```cpp
if (client.IsConnected()) {
    if (!client.IncomingTcpMessages().empty()) {
        auto msg = client.IncomingTcpMessages().pop_front().msg;
        // Handle the message
    }
}
```

### Sending messages

To send a message you need to create a message and then call the Send method.

```cpp
RType::net::Message<MessageType> msg;
msg.header.id = MessageType::Message1;
client.Send(msg);
```

## Creating a message

To create a message you need to create a struct that will contain the data you want to send.

```cpp
struct Message1 {
    int32_t x;
    int32_t y;
};

struct Message2 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};
```

Then use the RType::net::Message struct to create the message.

```cpp
RType::net::Message<MessageType> msg;
msg.header.id = MessageType::Message1;
msg << Message1{10, 20};
```

You can pass multiple structs to the operator<< to add multiple structs to the message.

```cpp
RType::net::Message<MessageType> msg;
// You must set the message id before adding the structs
msg.header.id = MessageType::Message1;
msg << Message1{10, 20} << Message2{30, 40};
```


## Unpacking a message

To unpack a message you need to create a struct that will contain the data you want to receive.

```cpp
struct Message1 {
    int32_t x;
    int32_t y;
};

struct Message2 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};
```

Then use the RType::net::Message struct to unpack the message.

```cpp
struct Message1 messageData;

RType::net::Message<MessageType> receivedMsg; // This is the message you received
receivedMsg >> messageData;
```

You can pass multiple structs to the operator>> to unpack multiple structs from the message.

```cpp
struct Message1 messageData1;
struct Message2 messageData2;

RType::net::Message<MessageType> receivedMsg; // This is the message you received
receivedMsg >> messageData1 >> messageData2;
```

<div class="section_buttons">
| Previous          |                              Next |
|:------------------|----------------------------------:|
| [Get Started](GettingStarted.md) | [UDP Tutorial](UDPModule.md) |
</div>
