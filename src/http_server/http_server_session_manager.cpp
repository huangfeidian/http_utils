

#include "http_server_session_manager.h"

namespace spiritsaway::http_utils
{

	http_server_session_manager::http_server_session_manager()
	{
	}

	void http_server_session_manager::start(http_server_session_ptr c)
	{
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_sessions.insert(c);
		}
		c->start();
	}

	void http_server_session_manager::stop(http_server_session_ptr c)
	{
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_sessions.erase(c);
		}

		c->stop();
	}

	void http_server_session_manager::stop_all()
	{
		std::vector<http_server_session_ptr> con_copys;
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
	std::size_t http_server_session_manager::get_session_count()
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		return m_sessions.size();
	}

} 
