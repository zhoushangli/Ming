#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

struct NetworkSystemConfig
{
	bool m_isEnable = true;
};

class NetworkSystem : public SystemBase
{
public:
	enum class NetworkState
	{
		Inactive,
		Idle,
		ServerListening,
		ClientConnecting,
		ClientConnected,
	};

	NetworkSystem(NetworkSystemConfig const& config) : m_config(config) {}
	virtual ~NetworkSystem() = default;

	virtual void Startup() override;
	virtual void Shutdown() override;
	virtual void BeginFrame() override;
	virtual void EndFrame() override;

	bool StartServer(uint16_t port);
	void StopServer();

	bool StartClient(std::string const& ipAddress, uint16_t port);
	void StopClient();
	void StopClient(std::size_t clientIndex);

	bool                     SendStringToAll(std::string const& text);
	std::vector<std::string> ConsumeReceivedStrings(std::size_t connectionIndex);

	NetworkState GetState() const { return m_state; }
	std::size_t  GetConnectionCount() const { return m_connections.size(); }

private:
	using SocketHandle = uintptr_t;
	static constexpr SocketHandle INVALID_SOCKET_HANDLE = ~SocketHandle(0);

	struct NetworkConnection
	{
		SocketHandle        m_socket = INVALID_SOCKET_HANDLE;
		std::deque<uint8_t> m_incomingBytes;
		std::deque<uint8_t> m_outgoingBytes;
	};

	void AcceptNewClients();
	void UpdateClientConnection();
	void ProcessConnections();
	bool SendQueuedData(NetworkConnection& connection);
	bool ReceiveIncomingData(NetworkConnection& connection);
	void CloseConnection(std::size_t connectionIndex);
	void CloseAllConnections();

private:
	NetworkSystemConfig m_config;

	NetworkState                   m_state        = NetworkState::Inactive;
	SocketHandle                   m_listenSocket = INVALID_SOCKET_HANDLE;
	std::vector<NetworkConnection> m_connections;
};
