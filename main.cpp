#include <chrono>
#include <functional>
#include <iostream>
#include <random>

#include "parallel_natural_merge_sort.h"

/*******************************************************************************
* Creates a random integer array of length 'length', minimum integer           *
* 'minimum', maximum integer 'maximum', using seed 'seed'.                     *
*******************************************************************************/
static int* get_random_int_array(const size_t length,
                                 const int minimum,
                                 const int maximum,
                                 const unsigned int seed)
{
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(minimum, maximum);

    int* array = new int[length];

    for (size_t i = 0; i < length; ++i)
    {
        array[i] = distribution(generator);
    }

    return array;
}

/*******************************************************************************
* Create an array of pointers to integers.                                     *
*******************************************************************************/
static int** get_random_int_pointer_array(const size_t length,
                                          const int minimum,
                                          const int maximum,
                                          const unsigned seed)
{
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(minimum, maximum);

    int** array = new int*[length];

    for (size_t i = 0; i < length; ++i)
    {
        array[i] = new int(distribution(generator));
    }

    return array;
}

/*******************************************************************************
* Returns a strongly presorted array of integers.                              *
*******************************************************************************/
static int* get_presorted_int_array(const size_t length)
{
    int* array = new int[length];
    int num = 0;

    for (size_t i = 0; i < length / 2; ++i)
    {
        array[i] = num++;
    }

    for (size_t i = length / 2; i < length; ++i)
    {
        array[i] = num--;
    }

    return array;
}

/*******************************************************************************
* Returns the milliseconds since the Unix epoch.                               *
*******************************************************************************/
static unsigned long long get_milliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::system_clock::now().time_since_epoch()).count();
}

/*******************************************************************************
* Profiles the 'std::stable_sort' agains the range ['begin', 'end') using the  *
* comparator 'cmp'.                                                            *
*******************************************************************************/
template<class T, class Cmp>
static void profile_stable_sort(T begin, T end, Cmp cmp)
{
    unsigned long long ta = get_milliseconds();
    std::stable_sort(begin, end, cmp);
    unsigned long long tb = get_milliseconds();

    std::cout << "std::stable_sort in "
              << (tb - ta)
              << " milliseconds. "
              << "Sorted: "
              << std::is_sorted(begin, end, cmp)
              << std::endl;
}

/*******************************************************************************
* Profiles the 'natural_merge_sort' agains the range ['begin', 'end') using    *
* the comparator 'cmp'.                                                        *
*******************************************************************************/
template<class T, class Cmp>
void profile_natural_merge_sort(T begin, T end, Cmp cmp)
{
    unsigned long long ta = get_milliseconds();
    natural_merge_sort(begin, end, cmp);
    unsigned long long tb = get_milliseconds();

    std::cout << "natural_merge_sort in "
              << (tb - ta)
              << " milliseconds. "
              << "Sorted: "
              << std::is_sorted(begin, end, cmp)
              << std::endl;
}

/*******************************************************************************
* Profiles the 'natural_merge_sort' agains the range ['begin', 'end') using    *
* the comparator 'cmp'.                                                        *
*******************************************************************************/
template<class T, class Cmp>
void profile_parallel_natural_merge_sort(T begin, T end, Cmp cmp)
{
    unsigned long long ta = get_milliseconds();
    parallel_natural_merge_sort(begin, end, cmp);
    unsigned long long tb = get_milliseconds();

    std::cout << "parallel_natural_merge_sort in "
              << (tb - ta)
              << " milliseconds. "
              << "Sorted: "
              << std::is_sorted(begin, end, cmp)
              << std::endl;
}

/*******************************************************************************
* Profiles the sorting algorithms on a random integer array.                   *
*******************************************************************************/
static void profile_on_random_array(const size_t sz,
                                    const int minimum,
                                    const int maximum,
                                    const unsigned seed)
{
    int* array1 = get_random_int_array(sz, minimum, maximum, seed);
    int* array2 = new int[sz];
    int* array3 = new int[sz];

    std::copy(array1, array1 + sz, array2);
    std::copy(array1, array1 + sz, array3);

    std::cout << "--- PROFILING ON RANDOM ARRAY OF LENGTH "
              << sz
              << " ---"
              << std::endl;

    profile_stable_sort(array1, 
                        array1 + sz, 
                        std::less<int>());

    profile_natural_merge_sort(array2, 
                               array2 + sz, 
                               std::less<int>());

    profile_parallel_natural_merge_sort(array3,
                                        array3 + sz,
                                        std::less<int>());

    std::cout << "Same contents: "
              << (std::equal(array1, array1 + sz, array2)
                 && std::equal(array1, array1 + sz, array3))
              << std::endl
              << std::endl;
}

/*******************************************************************************
* Profiles the sorting algorithms on an array of pointers to random integers.  *
*******************************************************************************/
static void profile_on_integer_pointer_array(const size_t sz,
                                             const int minimum,
                                             const int maximum,
                                             const unsigned seed)
{
    std::cout << "--- PROFILING ON RANDOM POINTER ARRAY OF LENGTH "
              << sz
              << " ---"
              << std::endl;

    int** array1 = get_random_int_pointer_array(sz,
                                                minimum,
                                                maximum,
                                                seed);
    int** array2 = new int*[sz];
    int** array3 = new int*[sz];

    std::copy(array1, array1 + sz, array2);
    std::copy(array1, array1 + sz, array3);

    auto lam = [](int* a, int* b) { return *a < *b; };
    
    profile_stable_sort(array1, 
                        array1 + sz, 
                        lam);

    profile_natural_merge_sort(array2, 
                               array2 + sz,
                               lam);

    profile_parallel_natural_merge_sort(array3,
                                        array3 + sz,
                                        lam);
    std::cout << "Same contents: "
              << (std::equal(array1, array1 + sz, array2)
                 && std::equal(array1, array1 + sz, array3))
              << std::endl
              << std::endl;
}

/*******************************************************************************
* Profiles the sorting algorithms on a presorted array.                        *
*******************************************************************************/
static void profile_on_presorted_array(const size_t sz)
{
    std::cout << "--- PROFILING ON PRESORTED ARRAY OF LENGTH "
              << sz
              << " ---"
              << std::endl;

    int* array1 = get_presorted_int_array(sz);
    int* array2 = new int[sz];
    int* array3 = new int[sz];

    std::copy(array1, array1 + sz, array2);
    std::copy(array1, array1 + sz, array3);

    profile_stable_sort(array1, 
                        array1 + sz, 
                        std::less<int>());

    profile_natural_merge_sort(array2, 
                               array2 + sz, 
                               std::less<int>());

    profile_parallel_natural_merge_sort(array3,
                                        array3 + sz,
                                        std::less<int>());

    std::cout << "Same contents: "
              << (std::equal(array1, array1 + sz, array2) 
                 && std::equal(array1, array1 + sz, array3))
              << std::endl
              << std::endl;
}

/*******************************************************************************
* The entry point to a demo program.                                           *
*******************************************************************************/
int main(int argc, const char * argv[]) {
    unsigned long long seed = get_milliseconds();

    std::cout << "Seed: "
              << seed
              << std::endl
              << std::endl;

    const size_t length = 5000000;
    const int min_int = -100;
    const int max_int = 300;

    std::cout << std::boolalpha;

    profile_on_random_array(length, min_int, max_int, seed);
    profile_on_integer_pointer_array(length, min_int, max_int, seed);
    profile_on_presorted_array(length);

    return 0;
}
