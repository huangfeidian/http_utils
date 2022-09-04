#pragma once

#include <set>
#include "http_server_session.h"
#include <mutex>

namespace spiritsaway::http_utils
{
	/// Manages open http_server_sessions so that they may be cleanly stopped when the server
	/// needs to shut down.
	class http_server_session_manager
	{
	public:
		http_server_session_manager(const http_server_session_manager &) = delete;
		http_server_session_manager &operator=(const http_server_session_manager &) = delete;

		/// Construct a http_server_session manager.
		http_server_session_manager();

		/// Add the specified http_server_session to the manager and start it.
		void start(http_server_session_ptr c);

		/// Stop the specified http_server_session.
		void stop(http_server_session_ptr c);

		/// Stop all http_server_sessions.
		void stop_all();
		
		std::size_t get_session_count();

	private:
		/// The managed http_server_sessions.
		std::set<http_server_session_ptr> m_sessions;
		std::mutex m_mutex;
	};
} // namespace spiritsaway::http_server
