
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <vector>


// g++-4.8 -std=c++11 -Wall -Wextra -pedantic -o triplets triplets.cpp

int main() {
    unsigned long N = 0;
    std::cin >> N;

    // If the sequence does not hold enough values for a single triplet,
    // handle it right now.
    if (N < 3)
        std::cout << 0;

    // Load the sequence of integers.
    // Note: We could insert them in a sorted and uniqued fashion to save
    //       much work later on. We could use a set, but it would be
    //       suboptimal because we're only storing integers.

    std::vector<unsigned long> d;
    d.reserve(N);
    std::copy(std::istream_iterator<unsigned long>(std::cin),
              std::istream_iterator<unsigned long>(),
              std::back_inserter(d));
    assert(N == d.size());
    std::copy(d.begin(), d.end(), std::ostream_iterator<unsigned long>(std::cerr, " "));
    std::cerr << '\n';

    // Sort them O(n log n)
    std::sort(d.begin(), d.end());
    std::copy(d.begin(), d.end(), std::ostream_iterator<unsigned long>(std::cerr, " "));
    std::cerr << '\n';

    // Eliminate duplicates and erase them from the sequence accordingly ~O(n).
    // considering unsigned longs are trivial to destroy, the erase should only
    // update the size of the vector and so it should be O(1).
    d.erase(std::unique(d.begin(), d.end()), d.end());
    std::copy(d.begin(), d.end(), std::ostream_iterator<unsigned long>(std::cerr, " "));
    std::cerr << '\n';

    // N will now be the length of the 'cleaned up' sequence.
    N = d.size();

    // Now, we observe that we only need to compute:
    // sum from i=2 to N-1 of (i-1)*(N-i)
    // after some mathematical simplification, we end up with
    // the following formula:
    // (1/6)N(N^2 - 3N + 2) = (N((N-3)N + 2)) / 6
    // (you can paste the sum in wolfram to get the simplification)
    unsigned long const triplets = (N * ((N - 3) * N + 2)) / 6;
    std::cout << triplets;

    // std::vector<unsigned long>::const_iterator i, last = d.end();
    // unsigned long triplets = 0;
    // unsigned long smaller = 1, larger = N - 2;
    // for (i = d.begin() + 1; i != last; ++i) {
    //     assert(larger > 0 && smaller > 0);

    //     triplets += smaller * larger;
    //     ++smaller;
    //     --larger;
    // }

    // std::cout << triplets;
}
