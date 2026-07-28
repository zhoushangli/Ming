#include "MingEngine/Engine/Network/NetworkSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include <algorithm>
#include <array>
#include <utility>

// clang-format off

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <WinSock2.h>
#include <WS2TCPIP.h>
#pragma comment(lib, "Ws2_32.lib")

// clang-format on

namespace
{

constexpr std::size_t NETWORK_BUFFER_SIZE = 2048;

} // namespace

void NetworkSystem::Startup()
{
	if (m_state != NetworkState::Inactive)
	{
		GUARANTEE_OR_DIE(false, "NetworkSystem is already started.");
	}

	WSADATA data;
	int const result = WSAStartup(MAKEWORD(2, 2), &data);
	GUARANTEE_OR_DIE(result == 0, Stringf("WSAStartup failed with error: %d", result));

	m_state = NetworkState::Idle;
}

void NetworkSystem::Shutdown()
{
	if (m_state == NetworkState::Inactive)
	{
		ERROR_AND_DIE("NetworkSystem is not started.");
	}

	if (m_state == NetworkState::ServerListening)
	{
		StopServer();
	}
	else if (m_state == NetworkState::ClientConnecting || m_state == NetworkState::ClientConnected)
	{
		StopClient();
	}

	int const result = WSACleanup();
	GUARANTEE_RECOVERABLE(result == 0, Stringf("WSACleanup failed with error: %d", WSAGetLastError()));

	m_state = NetworkState::Inactive;
}

void NetworkSystem::BeginFrame()
{
	switch (m_state)
	{
	case NetworkState::ServerListening:
		AcceptNewClients();
		ProcessConnections();
		break;

	case NetworkState::ClientConnecting:
		UpdateClientConnection();
		if (m_state == NetworkState::ClientConnected)
		{
			ProcessConnections();
		}
		break;

	case NetworkState::ClientConnected:
		ProcessConnections();
		break;

	default:
		break;
	}
}

void NetworkSystem::EndFrame() {}

bool NetworkSystem::StartServer(uint16_t port)
{
	if (m_state != NetworkState::Idle)
	{
		ERROR_RECOVERABLE("NetworkSystem must be idle before starting a server.");
		return false;
	}

	SOCKET const listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		ERROR_RECOVERABLE(Stringf("Failed to create listen socket. Error code: %d", WSAGetLastError()));
		return false;
	}

	unsigned long blockingMode = 1;
	int result = ioctlsocket(listenSocket, FIONBIO, &blockingMode);
	if (result == SOCKET_ERROR)
	{
		int const errorCode = WSAGetLastError();
		closesocket(listenSocket);
		ERROR_RECOVERABLE(Stringf("Failed to make listen socket non-blocking. Error code: %d", errorCode));
		return false;
	}

	sockaddr_in address          = {};
	address.sin_family           = AF_INET;
	address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	address.sin_port             = htons(port);

	result = bind(listenSocket, reinterpret_cast<sockaddr*>(&address), static_cast<int>(sizeof(address)));
	if (result == SOCKET_ERROR)
	{
		int const errorCode = WSAGetLastError();
		closesocket(listenSocket);
		ERROR_RECOVERABLE(Stringf("Failed to bind listen socket. Error code: %d", errorCode));
		return false;
	}

	result = listen(listenSocket, SOMAXCONN);
	if (result == SOCKET_ERROR)
	{
		int const errorCode = WSAGetLastError();
		closesocket(listenSocket);
		ERROR_RECOVERABLE(Stringf("Failed to listen on socket. Error code: %d", errorCode));
		return false;
	}

	m_listenSocket = static_cast<SocketHandle>(listenSocket);
	m_state        = NetworkState::ServerListening;
	return true;
}

void NetworkSystem::StopServer()
{
	CloseAllConnections();

	if (m_listenSocket != INVALID_SOCKET_HANDLE)
	{
		closesocket(static_cast<SOCKET>(m_listenSocket));
		m_listenSocket = INVALID_SOCKET_HANDLE;
	}

	if (m_state != NetworkState::Inactive)
	{
		m_state = NetworkState::Idle;
	}
}

bool NetworkSystem::StartClient(std::string const& ipAddress, uint16_t port)
{
	if (m_state != NetworkState::Idle)
	{
		ERROR_RECOVERABLE("NetworkSystem must be idle before starting a client.");
		return false;
	}

	sockaddr_in serverAddress = {};
	serverAddress.sin_family  = AF_INET;
	serverAddress.sin_port    = htons(port);

	int const addressResult = inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr);
	if (addressResult != 1)
	{
		ERROR_RECOVERABLE(Stringf("Invalid server IPv4 address: %s", ipAddress.c_str()));
		return false;
	}

	SOCKET const serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		ERROR_RECOVERABLE(Stringf("Failed to create client socket. Error code: %d", WSAGetLastError()));
		return false;
	}

	unsigned long blockingMode = 1;
	int result = ioctlsocket(serverSocket, FIONBIO, &blockingMode);
	if (result == SOCKET_ERROR)
	{
		int const errorCode = WSAGetLastError();
		closesocket(serverSocket);
		ERROR_RECOVERABLE(Stringf("Failed to make client socket non-blocking. Error code: %d", errorCode));
		return false;
	}

	result = connect(
		serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), static_cast<int>(sizeof(serverAddress)));
	if (result == SOCKET_ERROR)
	{
		int const errorCode = WSAGetLastError();
		if (errorCode != WSAEWOULDBLOCK)
		{
			closesocket(serverSocket);
			ERROR_RECOVERABLE(Stringf("Failed to begin client connection. Error code: %d", errorCode));
			return false;
		}
	}

	NetworkConnection connection;
	connection.m_socket = static_cast<SocketHandle>(serverSocket);
	m_connections.push_back(std::move(connection));
	m_state = result == 0 ? NetworkState::ClientConnected : NetworkState::ClientConnecting;
	return true;
}

void NetworkSystem::StopClient()
{
	if (m_state != NetworkState::ClientConnecting && m_state != NetworkState::ClientConnected)
	{
		return;
	}

	CloseAllConnections();
	m_state = NetworkState::Idle;
}

void NetworkSystem::StopClient(std::size_t clientIndex)
{
	if (m_state != NetworkState::ServerListening || clientIndex >= m_connections.size())
	{
		return;
	}

	CloseConnection(clientIndex);
}

bool NetworkSystem::SendStringToAll(std::string const& text)
{
	if (m_connections.empty())
	{
		return false;
	}

	for (NetworkConnection& connection : m_connections)
	{
		for (char character : text)
		{
			connection.m_outgoingBytes.push_back(static_cast<uint8_t>(character));
		}
		connection.m_outgoingBytes.push_back('\0');
	}

	return true;
}

std::vector<std::string> NetworkSystem::ConsumeReceivedStrings(std::size_t connectionIndex)
{
	std::vector<std::string> messages;
	if (connectionIndex >= m_connections.size())
	{
		return messages;
	}

	std::deque<uint8_t>& incomingBytes = m_connections[connectionIndex].m_incomingBytes;
	while (true)
	{
		auto const terminator = std::find(incomingBytes.begin(), incomingBytes.end(), uint8_t{ 0 });
		if (terminator == incomingBytes.end())
		{
			break;
		}

		std::string message;
		message.reserve(static_cast<std::size_t>(std::distance(incomingBytes.begin(), terminator)));
		while (!incomingBytes.empty() && incomingBytes.front() != 0)
		{
			message.push_back(static_cast<char>(incomingBytes.front()));
			incomingBytes.pop_front();
		}
		incomingBytes.pop_front();
		messages.push_back(std::move(message));
	}

	return messages;
}

void NetworkSystem::AcceptNewClients()
{
	while (true)
	{
		SOCKET const newClientSocket = accept(static_cast<SOCKET>(m_listenSocket), nullptr, nullptr);
		if (newClientSocket == INVALID_SOCKET)
		{
			int const errorCode = WSAGetLastError();
			if (errorCode != WSAEWOULDBLOCK)
			{
				ERROR_RECOVERABLE(Stringf("Failed to accept new client. Error code: %d", errorCode));
			}
			return;
		}

		unsigned long blockingMode = 1;
		int const result = ioctlsocket(newClientSocket, FIONBIO, &blockingMode);
		if (result == SOCKET_ERROR)
		{
			int const errorCode = WSAGetLastError();
			closesocket(newClientSocket);
			ERROR_RECOVERABLE(Stringf("Failed to make client socket non-blocking. Error code: %d", errorCode));
			continue;
		}

		NetworkConnection connection;
		connection.m_socket = static_cast<SocketHandle>(newClientSocket);
		m_connections.push_back(std::move(connection));
	}
}

void NetworkSystem::UpdateClientConnection()
{
	if (m_connections.empty())
	{
		m_state = NetworkState::Idle;
		return;
	}

	SOCKET const serverSocket = static_cast<SOCKET>(m_connections[0].m_socket);

	fd_set writeSockets;
	fd_set exceptSockets;
	FD_ZERO(&writeSockets);
	FD_ZERO(&exceptSockets);
	FD_SET(serverSocket, &writeSockets);
	FD_SET(serverSocket, &exceptSockets);

	timeval waitTime = {};
	int const result = select(0, nullptr, &writeSockets, &exceptSockets, &waitTime);
	if (result == SOCKET_ERROR)
	{
		int const errorCode = WSAGetLastError();
		StopClient();
		ERROR_RECOVERABLE(Stringf("Failed to check client connection. Error code: %d", errorCode));
		return;
	}
	if (result == 0)
	{
		return;
	}
	if (FD_ISSET(serverSocket, &exceptSockets))
	{
		StopClient();
		ERROR_RECOVERABLE("Client connection attempt failed.");
		return;
	}
	if (FD_ISSET(serverSocket, &writeSockets))
	{
		m_state = NetworkState::ClientConnected;
	}
}

void NetworkSystem::ProcessConnections()
{
	std::size_t connectionIndex = 0;
	while (connectionIndex < m_connections.size())
	{
		NetworkConnection& connection = m_connections[connectionIndex];
		if (!SendQueuedData(connection) || !ReceiveIncomingData(connection))
		{
			CloseConnection(connectionIndex);
			if (m_state == NetworkState::ClientConnected)
			{
				m_state = NetworkState::Idle;
			}
			continue;
		}

		++connectionIndex;
	}
}

bool NetworkSystem::SendQueuedData(NetworkConnection& connection)
{
	if (connection.m_outgoingBytes.empty())
	{
		return true;
	}

	std::array<char, NETWORK_BUFFER_SIZE> sendBuffer = {};
	std::size_t const byteCount = std::min(sendBuffer.size(), connection.m_outgoingBytes.size());
	for (std::size_t byteIndex = 0; byteIndex < byteCount; ++byteIndex)
	{
		sendBuffer[byteIndex] = static_cast<char>(connection.m_outgoingBytes[byteIndex]);
	}

	int const result = send(
		static_cast<SOCKET>(connection.m_socket), sendBuffer.data(), static_cast<int>(byteCount), 0);
	if (result == SOCKET_ERROR)
	{
		int const errorCode = WSAGetLastError();
		if (errorCode == WSAEWOULDBLOCK)
		{
			return true;
		}

		ERROR_RECOVERABLE(Stringf("Failed to send network data. Error code: %d", errorCode));
		return false;
	}
	if (result == 0)
	{
		return false;
	}

	for (int sentByteIndex = 0; sentByteIndex < result; ++sentByteIndex)
	{
		connection.m_outgoingBytes.pop_front();
	}
	return true;
}

bool NetworkSystem::ReceiveIncomingData(NetworkConnection& connection)
{
	std::array<char, NETWORK_BUFFER_SIZE> receiveBuffer = {};
	while (true)
	{
		int const result = recv(
			static_cast<SOCKET>(connection.m_socket),
			receiveBuffer.data(),
			static_cast<int>(receiveBuffer.size()),
			0);
		if (result > 0)
		{
			for (int receivedByteIndex = 0; receivedByteIndex < result; ++receivedByteIndex)
			{
				connection.m_incomingBytes.push_back(static_cast<uint8_t>(receiveBuffer[receivedByteIndex]));
			}
			continue;
		}
		if (result == 0)
		{
			return false;
		}

		int const errorCode = WSAGetLastError();
		if (errorCode == WSAEWOULDBLOCK)
		{
			return true;
		}

		ERROR_RECOVERABLE(Stringf("Failed to receive network data. Error code: %d", errorCode));
		return false;
	}
}

void NetworkSystem::CloseConnection(std::size_t connectionIndex)
{
	if (connectionIndex >= m_connections.size())
	{
		return;
	}

	closesocket(static_cast<SOCKET>(m_connections[connectionIndex].m_socket));
	m_connections.erase(m_connections.begin() + static_cast<std::ptrdiff_t>(connectionIndex));
}

void NetworkSystem::CloseAllConnections()
{
	for (NetworkConnection const& connection : m_connections)
	{
		closesocket(static_cast<SOCKET>(connection.m_socket));
	}
	m_connections.clear();
}
