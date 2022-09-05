#pragma once

#include <set>
#include <mutex>
#include <vector>
#include <cstdint>
#include <memory>

namespace spiritsaway::http_utils
{
	/// Manages open http_server_sessions so that they may be cleanly stopped when the server
	/// needs to shut down.
	template <typename T>
	class http_session_manager
	{
	public:
		http_session_manager(const http_session_manager &) = delete;
		http_session_manager &operator=(const http_session_manager &) = delete;

		/// Construct a http_server_session manager.
		http_session_manager()
		{
			
		}

		/// Add the specified http_server_session to the manager and start it.
		void start(std::shared_ptr<T> c)
		{
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_sessions.insert(c);
			}
			c->start();
		}

		/// Stop the specified http_server_session.
		void stop(std::shared_ptr<T> c)
		{
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_sessions.erase(c);
			}

			c->stop();
		}

		/// Stop all http_server_sessions.
		void stop_all()
		{
			std::vector<std::shared_ptr<T>> con_copys;
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				con_copys.insert(con_copys.end(), m_sessions.begin(), m_sessions.end());
				m_sessions.clear();
			}
			for (auto c : con_copys)
			{
				c->stop();
			}
		}
		
		std::size_t get_session_count()
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			return m_sessions.size();
		}

	private:
		/// The managed http_server_sessions.
		std::set<std::shared_ptr<T>> m_sessions;
		std::mutex m_mutex;
	};
} // namespace spiritsaway::http_server
