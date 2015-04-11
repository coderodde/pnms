#ifndef NATURAL_MERGE_SORT_H
#define NATURAL_MERGE_SORT_H

#include <algorithm>
#include <iterator>
#include <thread>
#include <vector>

/*******************************************************************************
* Implements a simple, array-based queue of integers. All three operations run *
* in constant time. This queue, however, does not check for under-/overflow of *
* underlying buffer because of performance considerations.                     *
*******************************************************************************/
class UnsafeIntQueue {
private:
    const size_t MINIMUM_CAPACITY = 256;

    size_t m_head;
    size_t m_tail;
    size_t m_size;
    size_t m_mask;
    size_t* m_buffer;

    /***************************************************************************
    * Makes sure a capacity is at least 'MINIMUM_CAPACITY' and is a power of   *
    * two.                                                                     *
    ***************************************************************************/
    size_t fixCapacity(size_t capacity)
    {
        capacity = std::max(capacity, MINIMUM_CAPACITY);
        size_t s = 1;

        while (s < capacity)
        {
            s <<= 1;
        }

        return s;
    }

public:

    /***************************************************************************
    * Constructs a new integer queue, which can accommodate 'capacit' amount   *
    * integers.                                                                *
    ***************************************************************************/
    UnsafeIntQueue(size_t capacity) :
    m_head{0},
    m_tail{0},
    m_size{0}
    {
        capacity = fixCapacity(capacity);
        m_mask = capacity - 1;
        m_buffer = new size_t[capacity];
    }

    /***************************************************************************
    * Destroys this queue, which releases the underlying buffer.               *
    ***************************************************************************/
    ~UnsafeIntQueue()
    {
        delete[] m_buffer;
    }

    /***************************************************************************
    * Appends the input integer to the tail of this queue.                     *
    ***************************************************************************/
    inline void enqueue(const size_t element)
    {
        m_buffer[m_tail & m_mask] = element;
        m_tail = (m_tail + 1) & m_mask;
        m_size++;
    }

    /***************************************************************************
    * Removes and returns the integer at the head of this queue.               *
    ***************************************************************************/
    inline size_t dequeue()
    {
        const size_t ret = m_buffer[m_head];
        m_head = (m_head + 1) & m_mask;
        m_size--;
        return ret;
    }

    /***************************************************************************
    * Returns the amount of integers in this queue.                            *
    ***************************************************************************/
    inline size_t size() const
    {
        return m_size;
    }
};

/*******************************************************************************
* Scans the range [first, last) and returns the queue containing sizes of each *
* run in the order they appear while scanning from left to right.              *
*******************************************************************************/
template<class RandomIt, class Cmp>
std::unique_ptr<UnsafeIntQueue> build_run_size_queue(RandomIt first,
                                                     RandomIt last,
                                                     Cmp cmp)
{
    const size_t length = std::distance(first, last);
    UnsafeIntQueue* p_q = new UnsafeIntQueue(length / 2 + 1);

    RandomIt head;
    RandomIt left = first;
    RandomIt right = left + 1;

    const RandomIt lst = last - 1;

    while (left < lst)
    {
        head = left;

        if (cmp(*right++, *left++))
        {
            // Reading a strictly descending run.
            while (left < lst && cmp(*right, *left))
            {
                ++left;
                ++right;
            }

            p_q->enqueue(right - head);
            std::reverse(head, right);
        }
        else
        {
            // Reading a ascending run.
            while (left < lst && !cmp(*right, *left))
            {
                ++left;
                ++right;
            }

            p_q->enqueue(left - head + 1);
        }

        ++left;
        ++right;
    }

    if (left == lst)
    {
        // Handle the case of an orphan element at the end of the range.
        p_q->enqueue(1);
    }

    return std::unique_ptr<UnsafeIntQueue>(p_q);
}

/*******************************************************************************
* Returns the amount of leading zeros in 'num'.                                *
*******************************************************************************/
size_t leading_zeros(const size_t num)
{
    size_t count = 0;

    for (size_t t = (size_t) 1 << (8 * sizeof(t) - 1); t; t >>= 1, ++count)
    {
        if ((t & num))
        {
            return count;
        }
    }

    return count;
}

/*******************************************************************************
* Returns the amount of merge passes needed to sort a range with 'run_amount'  *
* runs.                                                                        *
*******************************************************************************/
size_t get_pass_amount(size_t run_amount)
{
    return 8 * sizeof(run_amount) - leading_zeros(run_amount - 1);
}

/*******************************************************************************
* The actual implementation of natural merge sort.                             *
*******************************************************************************/
template<class RandomIt, class Cmp>
void natural_merge_sort_impl(RandomIt first, 
                             RandomIt last, 
                             RandomIt buffer,
                             Cmp cmp)
{
    const size_t length = std::distance(first, last);

    if (length < 2)
    {
        // Trivially sorted.
        return;
    }

    typedef typename std::iterator_traits<RandomIt>::value_type value_type;

    // Scan the runs.
    std::unique_ptr<UnsafeIntQueue> p_queue = build_run_size_queue(first, last, cmp);

    // Count the amount of merge passes over the array required to bring order.
    const size_t merge_passes = get_pass_amount(p_queue->size());

    RandomIt source;
    RandomIt target;

    // Make sure that after the last merge pass, all data ends up in the input
    // container.
    if ((merge_passes & 1) == 1)
    {
        source = buffer;
        target = first;
        std::copy(first, last, buffer);
    }
    else
    {
        source = first;
        target = buffer;
    }

    size_t runs_left = p_queue->size();
    size_t offset = 0;

    // While there is runs to merge, do...
    while (p_queue->size() > 1)
    {
        // Remove two runs from the head of the run queue.
        size_t left_run_length = p_queue->dequeue();
        size_t right_run_length = p_queue->dequeue();

        std::merge(source + offset,
                   source + offset + left_run_length,
                   source + offset + left_run_length,
                   source + offset + left_run_length + right_run_length,
                   target + offset,
                   cmp);

        // Append the merged run to the tail of the queue.
        p_queue->enqueue(left_run_length + right_run_length);
        runs_left -= 2;
        offset += left_run_length + right_run_length;

        // The current pass over the array is almost complete.
        switch (runs_left)
        {
            case 1:
            {
                const size_t single_length = p_queue->dequeue();

                std::copy(source + offset,
                          source + offset + single_length,
                          target + offset);

                p_queue->enqueue(single_length);
            }

            // FALL THROUGH!

            case 0:
            {
                runs_left = p_queue->size();
                offset = 0;
                RandomIt tmp = source;
                source = target;
                target = tmp;
                break;
            }
        }
    }
}

/*******************************************************************************
* Implements the natural merge sort, which sacrifices one pass over the input  *
* range in order to establish an implicit queue of runs. A run is the longest  *
* consecutive subsequence, in which all elements are ascending or strictly     *
* descending. Every descending run is reversed to ascending run. We cannot     *
* consider non-strictly descending runs, since that would sacrifice the stabi- *
* lity of the algorithm. After the run queue is establish, the algorithm re-   *
* moves two runs from the head of the queue, merges them into one run, which   *
* is then appended to the tail of the run queue. Merging continues until the   *
* queue contains only one run, which denotes that the entire input range is    *
* sorted.                                                                      *
*                                                                              *
* The best-case complexity is O(N), the average and worst-case complexity is   *
* O(N log N). Space complexity is O(N).                                        *
*******************************************************************************/
template<class RandomIt, class Cmp>
void natural_merge_sort(RandomIt first, RandomIt last, Cmp cmp)
{
    const size_t length = std::distance(first, last);

    if (length < 2)
    {
        // Trivially sorted.
        return;
    }

    typedef typename std::iterator_traits<RandomIt>::value_type value_type;
    RandomIt buffer = new value_type[length];
    natural_merge_sort_impl(first, last, buffer, cmp);
    delete[] buffer;
}

/*******************************************************************************
* Implements parallel merge sort.                                              *
*******************************************************************************/
template<class RandomIt, class Cmp>
void parallel_natural_merge_sort_impl(RandomIt source, 
                                      RandomIt target, 
                                      const size_t length, 
                                      const size_t thread_quota,
                                      Cmp cmp)
{
    if (thread_quota == 1)
    {
        natural_merge_sort_impl(target, target + length, source, cmp);
        return;
    }

    const size_t left_quota = thread_quota / 2;
    const size_t right_quota = thread_quota - left_quota;
    const size_t left_length = length / 2;

    if (thread_quota == 2)
    {
        std::thread thread_(natural_merge_sort_impl<RandomIt, Cmp>,
                            source,
                            source + left_length,
                            target,
                            cmp);

        natural_merge_sort_impl(source + left_length, 
                                source + length, 
                                target + left_length, 
                                cmp);

        thread_.join();

        std::merge(source, 
                   source + left_length, 
                   source + left_length, 
                   source + length,
                   target,
                   cmp);
        return;
    }

    std::thread left_thread(parallel_natural_merge_sort_impl<RandomIt, Cmp>,
                            target, 
                            source, 
                            left_length, 
                            left_quota, 
                            cmp);

    parallel_natural_merge_sort_impl(target + left_length, 
                                     source + left_length, 
                                     length - left_length, 
                                     right_quota, 
                                     cmp);
    // Wait for the left thread.
    left_thread.join();

    // Merge the two chunks.
    std::merge(source, 
               source + left_length, 
               source + left_length, 
               source + length,
               target,
               cmp);
}

/*******************************************************************************
* The actual parallel merge sort. If the system has N CPU cores, this sort     *     
* will split the range into N chunks of equal length assuming that N is a      *
* power of two, sort them concurrently and merge.                              *
*******************************************************************************/
template<class RandomIt, class Cmp>
void parallel_natural_merge_sort(RandomIt begin, RandomIt end, Cmp cmp)
{
    // At least 16384 elements per thread.
    constexpr size_t MINIMUM_THREAD_LOAD = 1 << 14;
    const size_t cores = std::thread::hardware_concurrency();
    const size_t length = std::distance(begin, end);
    const size_t spawn = std::min(cores, length / MINIMUM_THREAD_LOAD);

    if (spawn < 2)
    {
        natural_merge_sort(begin, end, cmp);
        return;
    }

    typedef typename std::iterator_traits<RandomIt>::value_type value_type;
    RandomIt buffer = new value_type[length];
    std::copy(begin, end, buffer);
    parallel_natural_merge_sort_impl(buffer, begin, length, spawn, cmp);
}

#endif
