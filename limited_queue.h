//////////////////////////////////////////////////////////////////////
/// \file  limited_queue.h
/// \brief depth-limited thread-safe queue container
///
///
/// This file contains definition of depth-limited thread-safe queue
/// container template class
///

#ifndef _LIMITED_QUEUE_H_
#define _LIMITED_QUEUE_H_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>

namespace vnn
{
/// Depth-limited thread-safe queue container with wait on push/pop operations
/// and optional element dropping when reaching the queue limit.
/// \tparam T - queue element type
template< typename T >
class LimitedQueue
{
public:
	/// Constructor
	/// \param[in] limit - queue depth limit, <= 0 for unlimited queue
	/// \param[in] allow_drop - allow dropping elements when reaching queue limit
	/// \param[in] timeout - wait timeout; 0 for infinite timeout
	LimitedQueue( size_t limit = 0, bool allow_drop = true, std::chrono::microseconds timeout = { 0 } ) :
		m_limit( limit ), m_allow_drop( allow_drop ), m_abort( false ), m_timeout( timeout )
	{}

	/// Push new element to the back of the queue (move semantic)
	/// \param[in] elem - element to push
	/// \return false if one of queue elements was discarded due to reaching the queue limit, or timeout happens
	bool push_back( T &&elem )
	{
		return emplace_back( std::move( elem ) );
	}

	/// Push new element to the back of the queue (copy semantic)
	/// \param[in] elem - element to push
	/// \return false if one of queue elements was discarded due to reaching the queue limit, or timeout happens
	bool push_back( const T &elem )
	{
		return emplace_back( elem );
	}

	/// Push new element to the back of the queue (construct in-place)
	/// \param[in] args - queue element constructor arguments
	/// \return false if one of queue elements was discarded due to reaching the queue limit, or timeout happens
	template< class... Args >
	bool emplace_back( Args &&...args )
	{
		{
			std::unique_lock< std::mutex > lk( m_mx );

			if( m_abort )
				return false;

			if( m_limit > 0 && m_queue.size() >= m_limit )
			{
				if( m_allow_drop )
				{
					m_queue.pop_front();
					m_queue.emplace_back( std::forward< Args >( args )... );
					return false;
				}

				auto wait_condition = [ & ] {
					return m_queue.size() < m_limit || m_abort;
				};

				if( m_timeout.count() > 0 )
				{
					if( !m_cv.wait_for( lk, m_timeout, wait_condition ) )
						return false;
				}
				else
					m_cv.wait( lk, wait_condition );

				// here we own mutex

				if( m_abort || m_queue.size() >= m_limit )
					return false;
			}
			m_queue.emplace_back( std::forward< Args >( args )... );
		}
		m_cv.notify_all();  // notify all waiting consumers that new element is in the queue
		return true;
	}

	/// Pop element from the front of the queue if queue is not empty.
	/// Wait for data arrival if queue is not empty.
	/// \param[out] elem - popped element
	/// \return false if the element was not retrieved
	bool pop_front( T &elem )
	{
		{
			std::unique_lock< std::mutex > lk( m_mx );

			if( m_abort )
				return false;

			if( m_queue.size() == 0 )
			{
				auto wait_condition = [ & ] {
					return m_queue.size() > 0 || m_abort;
				};

				if( m_timeout.count() > 0 )
				{
					if( !m_cv.wait_for( lk, m_timeout, wait_condition ) )
						return false;
				}
				else
					m_cv.wait( lk, wait_condition );

				// here we own mutex

				if( m_abort || m_queue.size() == 0 )
					return false;
			}

			elem = std::move( m_queue.front() );
			m_queue.pop_front();
		}
		m_cv.notify_all();  // notify all waiting producers that space in queue is available
		return true;
	}

	/// Get queue size
	/// \return queue size
	size_t size() const
	{
		std::unique_lock< std::mutex > lk( m_mx );
		return m_queue.size();
	}

	/// Check if queue is empty
	/// \return true if queue is empty
	bool empty() const
	{
		std::unique_lock< std::mutex > lk( m_mx );
		return m_queue.size() == 0;
	}

	/// Check if queue is full
	/// \return true if queue is full; unlimited queue is never full
	bool full() const
	{
		if( m_limit <= 0 )
			return false;
		std::unique_lock< std::mutex > lk( m_mx );
		return m_queue.size() >= m_limit;
	}

	/// Clear queue
	void clear()
	{
		std::unique_lock< std::mutex > lk( m_mx );
		m_queue.clear();
		m_cv.notify_all();  // notify all waiters to abort
	}

	/// Set termination flag
	void abort()
	{
		std::unique_lock< std::mutex > lk( m_mx );
		m_abort = true;
		m_cv.notify_all();  // notify all waiters to abort
	}

	/// Query termination flag
	/// \return true if termination was requested
	bool is_abort() const
	{
		return m_abort;
	}

private:
	std::deque< T > m_queue;              //!< queue container
	size_t m_limit;                       //!< queue size limit, or -1 for unlimited queue
	bool m_allow_drop;                    //!< allow dropping elements when reaching queue limit
	std::chrono::microseconds m_timeout;  //!< wait timeout
	mutable std::mutex m_mx;              //!< thread-protecting mutex
	std::condition_variable m_cv;         //!< condition variable to wait for data
	std::atomic_bool m_abort;             //!< termination request flag
};

}  // namespace vnn

#endif  //	_LIMITED_QUEUE_H_
