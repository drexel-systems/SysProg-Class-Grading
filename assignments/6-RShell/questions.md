1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

The remote client determines that a command's output is fully received by looking for a specific end-of-message (EOF) marker, like a special character (RDSH_EOF_CHAR) that the server sends when it's done sending output.

Since TCP is a stream-based protocol and doesn’t guarantee message boundaries, the client may receive partial reads of the command output. To handle this, the client should:

Loop until it receives the EOF marker, ensuring the full message is read.
Accumulate data from multiple recv() calls if the message is large.
Check if recv() returns 0, which means the server has closed the connection unexpectedly.

Without these techniques, the client might process incomplete output or get stuck waiting indefinitely.

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

TCP treats data as a continuous byte stream and does not inherently separate messages. A networked shell protocol needs to establish a clear way to delimit (mark the start and end of) each command to ensure proper parsing. One common approach is to use a special delimiter, such as a newline (\n) or an EOF character, so that the receiver knows when a full command has arrived. Another approach is length-prefixing, where the size of the message is sent first, allowing the receiver to expect and read a specific number of bytes before processing the command.

If message boundaries are not handled correctly, commands may be merged together, leading to incorrect execution, or they may be split across multiple recv() calls, causing incomplete execution. In some cases, the server may start processing a command before it is fully received, leading to errors or unintended behavior. Ensuring proper message framing is essential to avoid these issues and maintain a reliable communication channel between the client and the server

3. Describe the general differences between stateful and stateless protocols.

A stateful protocol maintains information about past interactions between the client and server, meaning that each request depends on previous ones. For example, a TCP connection is stateful because it establishes a persistent session where both parties keep track of the connection state, such as sequence numbers and acknowledgments. Stateful protocols often provide more advanced features, such as session management and connection persistence, but at the cost of requiring more resources on the server to track multiple clients.

In contrast, a stateless protocol treats each request as independent, meaning the server does not retain any information about past communications. A common example is HTTP (without cookies or sessions), where each request is processed in isolation without reference to previous interactions. Stateless protocols are generally more scalable because the server does not need to track client state, making them ideal for large-scale distributed systems. However, they can be less efficient in cases where remembering past interactions would be beneficial, such as in authentication, where the client may need to repeatedly send credentials with every request

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

UDP is considered unreliable because it does not guarantee packet delivery, order, or integrity. Unlike TCP, which establishes a connection and ensures that data is received in the correct order, UDP simply sends packets with no guarantee that they will arrive at all. However, despite its lack of reliability, UDP is often preferred in scenarios where low latency is more important than guaranteed delivery.

One of the main reasons to use UDP is for real-time applications, such as video streaming, VoIP (internet calls), and online gaming, where a slight delay in retransmitting lost packets would be worse than missing a few pieces of data. Additionally, UDP is commonly used in broadcast and multicast communications, where data needs to be sent to multiple clients simultaneously. Since UDP does not require the overhead of connection management, it is also more efficient for small, fast transmissions, such as DNS lookups, where a quick response is more important than reliability. In cases where reliability is needed, applications can implement their own error-checking and retransmission mechanisms on top of UDP rather than relying on the built-in features of TCP

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

The operating system provides the Berkeley Sockets API (or simply, "sockets") as the primary abstraction for network communication. This API allows applications to create and manage network connections without needing to understand the underlying hardware or protocols. Using the sockets API, applications can establish connections, send and receive data, and close network sessions using a set of standardized functions like socket(), bind(), connect(), send(), recv(), and close().

The sockets API supports both TCP (stream-based, reliable connections) and UDP (datagram-based, fast but unreliable transmissions), allowing applications to choose the appropriate protocol depending on their needs. By providing a consistent interface for networking, the sockets API enables developers to write networked applications that can communicate across different platforms and network environments without needing to handle the low-level details of packet transmission and routing :)