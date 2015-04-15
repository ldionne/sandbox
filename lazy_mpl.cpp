/*
time clang-3.5 -std=c++11 -Wall -Wextra -pedantic -o /dev/null -I ~/code/mpl11/include -c ~/code/sandbox/lazy_mpl.cpp -ftemplate-backtrace-limit=0 -stdlib=libc++ -I ~/code/libcxx/include
time clang++   -std=c++11 -Wall -Wextra -pedantic -o /dev/null -I ~/code/mpl11/include -c ~/code/sandbox/lazy_mpl.cpp -ftemplate-backtrace-limit=0 -stdlib=libc++
time g++-4.9   -std=c++11 -Wall -Wextra -pedantic -o /dev/null -I ~/code/mpl11/include -c ~/code/sandbox/lazy_mpl.cpp -ftemplate-backtrace-limit=0
*/


// Here, we could maybe use a special `F` to signal that we are done evaluating.
template <unsigned long N, typename F>
struct iterating_machine_impl {
    using next = typename iterating_machine_impl<
        N - N/2, typename iterating_machine_impl<N/2, F>::next
    >::next;
};

template <typename F> struct iterating_machine_impl<0, F> { using next = F; };
template <typename F> struct iterating_machine_impl<1, F> { using next = typename F::next; };
template <typename F> struct iterating_machine_impl<2, F> { using next = typename F::next::next; };
template <typename F> struct iterating_machine_impl<3, F> { using next = typename F::next::next::next; };
template <typename F> struct iterating_machine_impl<4, F> { using next = typename F::next::next::next::next; };
template <typename F> struct iterating_machine_impl<5, F> { using next = typename F::next::next::next::next::next; };
template <typename F> struct iterating_machine_impl<6, F> { using next = typename F::next::next::next::next::next::next; };
template <typename F> struct iterating_machine_impl<7, F> { using next = typename F::next::next::next::next::next::next::next; };
template <typename F> struct iterating_machine_impl<8, F> { using next = typename F::next::next::next::next::next::next::next::next; };
template <typename F> struct iterating_machine_impl<9, F> { using next = typename F::next::next::next::next::next::next::next::next::next; };

template <typename F>
struct iterating_machine {
    static constexpr unsigned long long MAX_RECURSION_DEPTH = 100000;
    using type = typename iterating_machine_impl<
        MAX_RECURSION_DEPTH, F
    >::next::type;
};



namespace lazy_mpl {
    // Apply, metafunctions and metafunction classes
    template <typename F, typename ...Args>
    struct apply {
        using type = typename F::type::template apply<Args...>::type;
    };

    template <template <typename ...> class F>
    struct quote {
        struct result {
            template <typename ...Args>
            using apply = F<Args...>;
        };

        using type = result;
    };

    template <typename X>
    struct id {
        using type = X;
    };

    template <typename X>
    struct always {
        struct result {
            template <typename ...>
            using apply = id<X>;
        };

        using type = result;
    };


    // Integrals

    template <typename T, T v>
    struct integral_c {
        struct result {
            using value_type = T;
            static constexpr value_type value = v;
        };

        using type = result;
    };

    template <bool B>
    using bool_ = integral_c<bool, B>;

    template <int I>
    using int_ = integral_c<int, I>;

    using true_ = bool_<true>;
    using false_ = bool_<false>;



    // If
    template <bool Condition, typename Then, typename Else>
    struct if_c { using type = typename Then::type; };

    template <typename Then, typename Else>
    struct if_c<false, Then, Else> { using type = typename Else::type; };

    template <typename Condition, typename Then, typename Else>
    struct if_ {
        using type = typename if_c<Condition::type::value, Then, Else>::type;
    };


    // Logicals
    template <typename X, typename Y>
    struct or_ {
        using type = typename if_<X, X, Y>::type;
    };

    template <typename X, typename Y>
    struct and_ {
        using type = typename if_<X, Y, X>::type;
    };

    template <typename X>
    struct not_ {
        using type = typename if_<X, true_, false_>::type;
    };


    // Sequences
    template <typename ...T>
    struct list {
        struct result { using is_empty = true_; };
        using type = result;
    };

    template <typename T, T ...v>
    using list_c = list<integral_c<T, v>...>;

    template <typename Head, typename ...Tail>
    struct list<Head, Tail...> {
        struct result {
            using head = Head;
            using tail = list<Tail...>;
            using is_empty = false_;
        };

        using type = result;
    };

    template <typename List>
    struct head {
        using type = typename List::type::head::type;
    };

    template <typename List>
    struct tail {
        using type = typename List::type::tail::type;
    };

    template <typename List>
    struct is_empty {
        using type = typename List::type::is_empty::type;
    };

    template <typename Head, typename Tail>
    struct cons {
        struct result {
            using head = Head;
            using tail = Tail;
            using is_empty = false_;
        };

        using type = result;
    };

    template <typename T>
    struct repeat {
        struct result {
            using head = T;
            using tail = repeat<T>;
            using is_empty = false_;
        };

        using type = result;
    };


    // Folds
    template <typename F>
    struct tail_recurse {
        using next = F;
    };

    template <typename F, typename State, typename Seq, bool = is_empty<Seq>::type::value>
    struct strict_foldl_impl {
        using type = typename State::type;
        using next = strict_foldl_impl;
    };

    template <typename F, typename State, typename Seq>
    struct strict_foldl_impl<F, State, Seq, false> {
    private:
        using xxx = typename apply<F, State, head<Seq>>::type;

    public:
        using next = typename tail_recurse<
            strict_foldl_impl<F, apply<F, State, head<Seq>>, tail<Seq>>
        >::next;
    };

    template <typename F, typename State, typename Seq>
    struct strict_foldl {
        using type = typename iterating_machine<
            strict_foldl_impl<F, State, Seq>
        >::type;
    };


    template <typename X>
    struct bottom_tail_recurse {
        using next = bottom_tail_recurse;
        using type = X;
    };

    template <typename F, typename State, typename Seq, bool = is_empty<Seq>::type::value>
    struct foldl_impl
        : bottom_tail_recurse<
            typename State::type
        >
    { };

    template <typename F, typename State, typename Seq>
    struct foldl_impl<F, State, Seq, false>
        : tail_recurse<
            foldl_impl<F, apply<F, State, head<Seq>>, tail<Seq>>
        >
    { };

    template <typename F, typename State, typename Seq>
    struct foldl {
        using type = typename iterating_machine<
            foldl_impl<F, State, Seq>
        >::type;
    };


    template <typename F, typename State, typename Seq, bool = is_empty<Seq>::type::value>
    struct foldr {
        using type = typename State::type;
    };

    template <typename F, typename State, typename Seq>
    struct foldr<F, State, Seq, false> {
        // If apply returned something lazy with a nested ::next member,
        // could we use it with iterating_machine?
        using type = typename apply<F, head<Seq>, foldr<F, State, tail<Seq>>>::type;
    };


    // Arithmetic
    template <typename X, typename Y>
    struct add {
        using type = typename integral_c<
            decltype(X::type::value + Y::type::value),
            X::type::value + Y::type::value
        >::type;
    };
}

namespace strict_mpl {
    // Metafunctions
    template <typename F, typename ...Args>
    struct apply {
        using type = typename F::template apply<Args...>::type;
    };

    template <template <typename ...> class F>
    struct quote {
        template <typename ...Args>
        using apply = F<Args...>;
    };

    template <bool Condition, typename Then, typename Else>
    struct if_c { using type = Then; };

    template <typename Then, typename Else>
    struct if_c<false, Then, Else> { using type = Else; };

    template <typename Condition, typename Then, typename Else>
    struct if_
        : if_c<Condition::value, Then, Else>
    { };

    // Integrals
    template <typename T, T v>
    struct integral_c {
        using value_type = T;
        static constexpr value_type value = v;
    };

    template <int I>
    using int_ = integral_c<int, I>;

    template <bool B>
    using bool_ = integral_c<bool, B>;

    using true_ = bool_<true>;
    using false_ = bool_<false>;


    // Logicals
    template <typename X, typename Y>
    struct or_ {
        using type = typename if_<X, X, Y>::type;
    };

    template <typename X, typename Y>
    struct and_ {
        using type = typename if_<X, Y, X>::type;
    };

    template <typename X>
    struct not_ {
        using type = typename if_<X, true_, false_>::type;
    };


    // Sequences
    template <typename ...T>
    struct list {
        using is_empty = true_;
    };

    template <typename T, T ...v>
    using list_c = list<integral_c<T, v>...>;

    template <typename Head, typename ...Tail>
    struct list<Head, Tail...> {
        using head = Head;
        using tail = list<Tail...>;
        using is_empty = false_;
    };

    template <typename List>
    struct head {
        using type = typename List::head;
    };

    template <typename List>
    struct tail {
        using type = typename List::tail;
    };

    template <typename List>
    struct is_empty {
        using type = typename List::is_empty;
    };

    // Arithmetic
    template <typename X, typename Y>
    struct add {
        using type = integral_c<
            decltype(X::value + Y::value), X::value + Y::value
        >;
    };


    // Folds
    template <typename F, typename State, typename Seq, bool = is_empty<Seq>::type::value>
    struct foldl_impl {
        using type = State;
        using next = foldl_impl;
    };

    template <typename F, typename State, typename Seq>
    struct foldl_impl<F, State, Seq, false> {
        using next = foldl_impl<
            F,
            typename apply<F, State, typename head<Seq>::type>::type,
            typename tail<Seq>::type
        >;
    };

    template <typename F, typename State, typename Seq>
    struct foldl {
        using type = typename iterating_machine<
            foldl_impl<F, State, Seq>
        >::type;
    };

    template <typename F, typename State, typename Seq>
    using strict_foldl = foldl<F, State, Seq>;

    template <typename F, typename State, typename Seq, bool = is_empty<Seq>::type::value>
    struct foldr { using type = State; };

    template <typename F, typename State, typename Seq>
    struct foldr<F, State, Seq, false> {
        using type = typename apply<
            F,
            typename head<Seq>::type,
            typename foldr<F, State, typename tail<Seq>::type>::type
        >::type;
    };
}


// using namespace strict_mpl;
using namespace lazy_mpl;

// attention:
// je pense que ce qui arrive ici est qu'on fold effectivement toute
// la liste mais and_<false_, undefined> retourne `false_`. bref, ca
// devrait chier sur des listes infinies.

// ruby -e "puts 500.times.collect { 'true_' }.join(', ')" | pbcopy

using And = foldl<
    quote<and_>,
    true_,
    list<
        true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_, true_
        , false_,
        struct undefined,
        struct undefined
    >
>::type;
// using xxxxx = And::foob;

// ruby -e "puts 0.upto(800).to_a.join(', ')" | pbcopy

// using Sum = strict_foldl<
//     quote<add>,
//     int_<0>,
//     list_c<int,
//         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 438, 439, 440, 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471, 472, 473, 474, 475, 476, 477, 478, 479, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495, 496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523, 524, 525, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538, 539, 540, 541, 542, 543, 544, 545, 546, 547, 548, 549, 550, 551, 552, 553, 554, 555, 556, 557, 558, 559, 560, 561, 562, 563, 564, 565, 566, 567, 568, 569, 570, 571, 572, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 586, 587, 588, 589, 590, 591, 592, 593, 594, 595, 596, 597, 598, 599, 600, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620, 621, 622, 623, 624, 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650, 651, 652, 653, 654, 655, 656, 657, 658, 659, 660, 661, 662, 663, 664, 665, 666, 667, 668, 669, 670, 671, 672, 673, 674, 675, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 688, 689, 690, 691, 692, 693, 694, 695, 696, 697, 698, 699, 700, 701, 702, 703, 704, 705, 706, 707, 708, 709, 710, 711, 712, 713, 714, 715, 716, 717, 718, 719, 720, 721, 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734, 735, 736, 737, 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753, 754, 755, 756, 757, 758, 759, 760, 761, 762, 763, 764, 765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776, 777, 778, 779, 780, 781, 782, 783, 784, 785, 786, 787, 788, 789, 790, 791, 792, 793, 794, 795, 796, 797, 798, 799, 800
//     >
// >::type;
