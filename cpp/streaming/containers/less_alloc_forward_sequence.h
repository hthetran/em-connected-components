/*
 * less_alloc_forward_sequence.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_LESS_ALLOC_SEQUENCE_H
#define EM_CC_LESS_ALLOC_SEQUENCE_H

#include <algorithm>
#include <deque>
#include <utility>

#include <tlx/define.hpp>
#include <tlx/logger/core.hpp>

#include <foxxll/common/tmeta.hpp>
#include <foxxll/mng/block_manager.hpp>
#include <foxxll/mng/prefetch_pool.hpp>
#include <foxxll/mng/read_write_pool.hpp>
#include <foxxll/mng/typed_block.hpp>
#include <foxxll/mng/write_pool.hpp>

#include <stxxl/bits/defines.h>
#include <stxxl/bits/deprecated.h>
#include <stxxl/types>

namespace stxxl {

template <class ValueType,
    size_t BlockSize = STXXL_DEFAULT_BLOCK_SIZE(ValueType),
    class AllocStr = foxxll::default_alloc_strategy,
    class SizeType = external_size_type>
class less_alloc_forward_sequence
{
    static constexpr bool debug = false;

public:
    using value_type = ValueType;
    using alloc_strategy_type = AllocStr;
    using size_type = SizeType;
    enum {
        block_size = BlockSize
    };

    using block_type = foxxll::typed_block<block_size, value_type>;
    using bid_type = foxxll::BID<block_size>;

    using bid_vector_type = std::vector<bid_type>;

private:
    using pool_type = foxxll::read_write_pool<block_type>;

    /// current number of items in the sequence
    size_type m_size;

    /// whether the m_pool object is own and should be deleted.
    bool m_owns_pool;

    /// read_write_pool of blocks
    pool_type* m_pool;

    /// current back block of sequence
    block_type* m_back_block;

    /// pointer to current back element in m_back_block
    value_type* m_back_element;

    /// block allocation strategy
    alloc_strategy_type m_alloc_strategy;

    /// block allocation counter
    size_t m_alloc_count;

    /// allocated block identifiers
    bid_vector_type m_bids;

    /// block manager used
    foxxll::block_manager* m_bm;

    /// number of blocks to prefetch
    size_t m_blocks2prefetch;

public:
    //! \name Constructors/Destructors
    //! \{

    //! Constructs empty sequence with own write and prefetch block pool
    //!
    //! \param D  number of parallel disks, defaulting to the configured number of scratch disks,
    //!           memory consumption will be 2 * D + 2 blocks
    //!           (first and last block, D blocks as write cache, D block for prefetching)
    explicit less_alloc_forward_sequence(const int D = -1)
        : m_size(0),
          m_owns_pool(true),
          m_alloc_count(0),
          m_bm(foxxll::block_manager::get_instance())
    {
        const size_t disks = (D < 1)
                             ? foxxll::config::get_instance()->disks_number()
                             : static_cast<size_t>(D);

        TLX_LOG << "sequence[" << this << "]::sequence(D)";
        m_pool = new pool_type(disks, disks + 2);
        init();
    }

    //! Constructs empty sequence with own write and prefetch block pool
    //!
    //! \param w_pool_size  number of blocks in the write pool, must be at least 2, recommended at least 3
    //! \param p_pool_size  number of blocks in the prefetch pool, recommended at least 1
    //! \param blocks2prefetch  defines the number of blocks to prefetch (\c front side),
    //!                          default is number of block in the prefetch pool
    explicit less_alloc_forward_sequence(const size_t w_pool_size, const size_t p_pool_size, int blocks2prefetch = -1)
        : m_size(0),
          m_owns_pool(true),
          m_alloc_count(0),
          m_bm(foxxll::block_manager::get_instance())
    {
        TLX_LOG << "sequence[" << this << "]::sequence(sizes)";
        m_pool = new pool_type(p_pool_size, w_pool_size);
        init(blocks2prefetch);
    }

    //! Constructs empty sequence
    //!
    //! \param pool block write/prefetch pool
    //! \param blocks2prefetch  defines the number of blocks to prefetch (\c front side), default is number of blocks in the prefetch pool
    //!  \warning Number of blocks in the write pool must be at least 2, recommended at least 3
    //!  \warning Number of blocks in the prefetch pool recommended at least 1
    explicit less_alloc_forward_sequence(pool_type& pool, int blocks2prefetch = -1)
        : m_size(0),
          m_owns_pool(false),
          m_pool(&pool),
          m_alloc_count(0),
          m_bm(foxxll::block_manager::get_instance())
    {
        TLX_LOG << "sequence[" << this << "]::sequence(pool)";
        init(blocks2prefetch);
    }

    //! non-copyable: delete copy-constructor
    less_alloc_forward_sequence(const less_alloc_forward_sequence&) = delete;
    //! non-copyable: delete assignment operator
    less_alloc_forward_sequence& operator = (const less_alloc_forward_sequence&) = delete;

    //! \}

    //! \name Modifiers
    //! \{

    void swap(less_alloc_forward_sequence& obj)
    {
        std::swap(m_size, obj.m_size);
        std::swap(m_owns_pool, obj.m_owns_pool);
        std::swap(m_pool, obj.m_pool);
        std::swap(m_back_block, obj.m_back_block);
        std::swap(m_back_element, obj.m_back_element);
        std::swap(m_alloc_strategy, obj.m_alloc_strategy);
        std::swap(m_alloc_count, obj.m_alloc_count);
        std::swap(m_bids, obj.m_bids);
        std::swap(m_bm, obj.m_bm);
        std::swap(m_blocks2prefetch, obj.m_blocks2prefetch);
    }

    //! \}

private:
    void init(int blocks2prefetch = -1)
    {
        if (m_pool->size_write() < 2) {
            TLX_LOG1 << "sequence: invalid configuration, not enough blocks (" << m_pool->size_write() <<
                     ") in write pool, at least 2 are needed, resizing to 3";
            m_pool->resize_write(3);
        }

        if (m_pool->size_write() < 3) {
            TLX_LOG1 << "sequence: inefficient configuration, no blocks for buffered writing available";
        }

        if (m_pool->size_prefetch() < 1) {
            TLX_LOG1 << "sequence: inefficient configuration, no blocks for prefetching available";
        }

        /// initialize empty less_alloc_forward_sequence
        m_back_block = m_pool->steal();
        m_back_element = m_back_block->begin() - 1;
        set_prefetch_aggr(blocks2prefetch);
    }

public:
    //! \name Miscellaneous
    //! \{

    //! Defines the number of blocks to prefetch (\c front side).
    //! This method should be called whenever the prefetch pool is resized
    //! \param blocks2prefetch  defines the number of blocks to prefetch (\c front side),
    //!                         a negative value means to use the number of blocks in the prefetch pool
    void set_prefetch_aggr(int blocks2prefetch)
    {
        if (blocks2prefetch < 0)
            m_blocks2prefetch = m_pool->size_prefetch();
        else
            m_blocks2prefetch = blocks2prefetch;
    }

    //! Returns the number of blocks prefetched from the \c front side
    const size_t & get_prefetch_aggr() const
    {
        return m_blocks2prefetch;
    }

    //! \}

    //! \name Modifiers
    //! \{

    void reset()
    {
        m_alloc_count = 0;
        m_size = 0;
        m_back_element = m_back_block->begin() - 1;

        for (auto& m_bid : m_bids)
            m_pool->invalidate(m_bid);

        if (!m_bids.empty())
            m_bm->delete_blocks(m_bids.begin(), m_bids.end());

        m_bids.clear();
    }

    //! Adds an element to the end of the sequence
    void push_back(const value_type& val)
    {
        if (TLX_UNLIKELY(m_back_element == m_back_block->begin() + (block_type::size - 1)))
        {
            TLX_LOG << "sequence::push_back Case 2";
            // write the back block
            // need to allocate new block
            bid_type newbid;

            m_bm->new_block(m_alloc_strategy, newbid, m_alloc_count++);

            TLX_LOG << "sequence[" << this << "]: push_back block " << m_back_block << " @ " << newbid;
            m_bids.push_back(newbid);
            m_pool->write(m_back_block, newbid);
            if (m_bids.size() <= m_blocks2prefetch) {
                TLX_LOG << "sequence::push_back Case Hints";
                m_pool->hint(newbid);
            }

            m_back_block = m_pool->steal();

            m_back_element = m_back_block->begin();
            *m_back_element = val;
            ++m_size;
        }
        else // not at end of a block
        {
            ++m_back_element;
            *m_back_element = val;
            ++m_size;
        }
    }
    //! \}

    //! \name Capacity
    //! \{

    //! Returns the size of the sequence
    size_type size() const
    {
        return m_size;
    }

    //! Returns \c true if sequence is empty
    bool empty() const
    {
        return (m_size == 0);
    }

    //! \}

    //! \name Operators
    //! \{

    //! Returns a mutable reference at the back of the sequence
    value_type & back()
    {
        assert(!empty());
        return *m_back_element;
    }

    //! Returns a const reference at the back of the sequence
    const value_type & back() const
    {
        assert(!empty());
        return *m_back_element;
    }

    //! \}

    //! \name Constructors/Destructors
    //! \{

    ~less_alloc_forward_sequence()
    {
        m_pool->add(m_back_block);

        if (m_owns_pool)
            delete m_pool;

        if (!m_bids.empty())
            m_bm->delete_blocks(m_bids.begin(), m_bids.end());
    }

    //! \}

    /**************************************************************************/

    class stream
    {
    public:
        using value_type = typename less_alloc_forward_sequence::value_type;

        using bid_iter_type = typename bid_vector_type::const_iterator;

    protected:
        const less_alloc_forward_sequence& m_sequence;

        size_type m_size;

        value_type* m_current_element;

        block_type* m_current_block;

        bid_iter_type m_next_bid;

    public:
        explicit stream(const less_alloc_forward_sequence& sequence)
            : m_sequence(sequence),
              m_size(sequence.size())
        {
            if (sequence.m_bids.empty()) {
                m_current_block = sequence.m_back_block;
                m_current_element = sequence.m_back_block->begin();
                m_next_bid = sequence.m_bids.end();
            } else {
                // get own block
                m_current_block = new block_type(); // m_sequence.m_pool->steal();
                m_next_bid = sequence.m_bids.begin();
                foxxll::request_ptr req = m_sequence.m_pool->read(m_current_block, *m_next_bid);
                TLX_LOG << "sequence[" << this << "]::stream::() read block " << m_current_block << " @ " << *m_next_bid;

                // give prefetching hints
                bid_iter_type bid = m_next_bid + 1;
                for (size_t i = 0; i < m_sequence.m_blocks2prefetch && bid != m_sequence.m_bids.end(); ++i, ++bid)
                {
                    TLX_LOG << "sequence::stream::operator++ giving prefetch hints";
                    m_sequence.m_pool->hint(*bid);
                }

                m_current_element = m_current_block->begin();
                req->wait();

                ++m_next_bid;
            }
        }

        ~stream() = default;

        //! return number of element left till end-of-stream.
        size_type size() const
        {
            return m_size;
        }

        //! standard stream method
        bool empty() const
        {
            return (m_size == 0);
        }

        //! standard stream method
        const value_type& operator * () const
        {
            assert(!empty());
            return *m_current_element;
        }

        //! standard stream method
        stream& operator ++ ()
        {
            assert(!empty());

            if (TLX_UNLIKELY(m_current_element == m_current_block->begin() + (block_type::size - 1)))
            {
                // next item position is beyond end of current block, find next block
                --m_size;

                if (m_size == 0)
                {
                    TLX_LOG << "sequence::stream::operator++ last block finished clean at block end";
                    assert(m_next_bid == m_sequence.m_bids.end());
                    assert(m_current_block == m_sequence.m_back_block);
                    // nothing to give back to sequence pool
                    m_current_element = nullptr;
                    return *this;
                }
                else if (m_size <= block_type::size)    // still items left in last partial block
                {
                    TLX_LOG << "sequence::stream::operator++ reached last block";
                    assert(m_next_bid == m_sequence.m_bids.end());
                    delete m_current_block;
                    m_current_block = m_sequence.m_back_block;
                    m_current_element = m_current_block->begin();
                    return *this;
                }

                TLX_LOG << "sequence::stream::operator++ default case: fetch next block";

                assert(m_next_bid != m_sequence.m_bids.end());
                foxxll::request_ptr req = m_sequence.m_pool->read(m_current_block, *m_next_bid);
                TLX_LOG << "sequence[" << this << "]::stream::operator++ read block " << m_current_block << " @ " << *m_next_bid;

                // give prefetching hints
                bid_iter_type bid = m_next_bid + 1;
                for (size_t i = 0; i < m_sequence.m_blocks2prefetch && bid != m_sequence.m_bids.end(); ++i, ++bid)
                {
                    TLX_LOG << "sequence::stream::operator++ giving prefetch hints";
                    m_sequence.m_pool->hint(*bid);
                }

                m_current_element = m_current_block->begin();
                req->wait();

                ++m_next_bid;
            }
            else
            {
                --m_size;
                ++m_current_element;
            }
            return *this;
        }
    };

    //! \name Miscellaneous
    //! \{

    //! construct a forward stream from this sequence
    stream get_stream()
    {
        return stream(*this);
    }
};

}

#endif //EM_CC_LESSALLOCSEQUENCE_H
