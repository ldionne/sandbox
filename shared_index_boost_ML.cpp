//////////////////////////////////////////////////////////////////////////////
// This is a reformulation of a post on the Boost Mailing list that was
// talking about shared indices. I tried to see what was the underlying
// principle because the idea was interesting, but I eventually lost interest.
//////////////////////////////////////////////////////////////////////////////






// The basic idea is that you have a bunch of keys stored in one object of type A:

struct A{
   vector<Key> keys;
};

// and then you have multiple objects of type B that each have to associate
// data with each key stored in A. you could use:

struct B{
   map<Key, Data> data;
};

// but since A::keys already provides a contiguous index for the keys you can
// just as well store the data in B in a vector with matching indexes, and
// make sure that the indexes used in A::keys don't change:

struct A{
   vector<Key> keys;
   vector<std::size_t> freelist;
   map<Key, std::size_t> index;

   void erase(...){ push index of unused key to freelist; }
   void push_back(Key){ reuse a freelist entry or keys.push_back; }
};

struct B{
   vector<optional<Data> > data;
};

// keep in mind that there is one object A for many objects B. you can now
// access the associated data of many objects B by looking up the index once
// in object A, and by wasting some space in B::data for unused entries:

b.data[a.index[key]]

// If Data is a pointer or any other type that knows an empty() state, you can
// also get rid of optional<> and generalize it as two template like:

template<class Key>
class index;

template<class Mapped, class OptionalTraits = boost_optional_traits>
class vector_map;



//////////////////////////////////////////////////////////////////////////////

template <typename Key>
struct SharedIndex {
    typedef Key key_type;
    typedef std::size_t mapped_type;

    mapped_type operator[](key_type const& key) {
        return indices_[key];
    }

private:
    typedef std::map<key_type, mapped_type> implementation_detail;
    implementation_detail indices_;
};


template <typename T, typename SharedIndex>
struct IndexedContent {
    typedef typename SharedIndex::key_type key_type;
    typedef typename T mapped_type;

    explicit IndexedContent(SharedIndex const& index)
        : index_(index)
    { }

    mapped_type& operator[](key_type const& key) {
        BOOST_ASSERT(index_.span() < values_.size());
        return values_[index_[key]];
    }

private:
    SharedIndex const& index_;

    typedef std::vector<value_type> implementation_detail;
    implementation_detail values_;
};
