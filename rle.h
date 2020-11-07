#pragma once

#include <cassert>
#include <list>
#include <vector>


namespace compress {
    // Needs to be packed because is directly written into file
    template<class base_t>
    struct rle_chunk {
        size_t _repetitions;
        base_t _value;

        operator base_t() const {
            return _value;
        }
        operator base_t&() {
            return _value;
        }
    } __attribute__((packed));

    template<class base_t>
    struct rle_meta {
        // chunk: how many times value exists ..
        std::list<rle_chunk<base_t>> _chunks = {};
        // original storage container size
        size_t _uncompressed_size = 0;
    };

    template<class base_t>
    class rle {
        rle_meta<base_t> _rle;
        using chunk_t = rle_chunk<base_t>;

    protected:
        void init(const base_t &val) {
            clear();
            _rle._uncompressed_size = 1;
            _rle._chunks = {{ 1, val }};
        }

    public:
        const auto &data() const {
            return _rle;
        }

        auto &data() {
            return _rle;
        }

        void clear() {
            _rle._uncompressed_size = 0;
            _rle._chunks.clear();
        }

        //! add value to rle
        void operator << (const base_t &val) {
            add(val);
        }

        //! add value to rle
        void add(const base_t &val) {
            if(_rle._chunks.empty() || _rle._uncompressed_size < 1) {
                init(val);
                return;
            }

            if(val == _rle._chunks.rbegin()->_value) {
                _rle._chunks.rbegin()->_repetitions++;
            }
            else {
                _rle._chunks.push_back({ 1, val });
            }
            _rle._uncompressed_size++;
        }

        const auto operator [] (const size_t id) const {
            return get(id);
        }

        //! returns an iterator to an element in the rle array at a given position
        //! iterator::end() if nothing was found
        auto get(const size_t id) const {
            size_t pos = 0;
            using cont_t = std::list<rle_chunk<base_t>>;
            typename cont_t::const_iterator it = _rle._chunks.begin();
            for (const auto &chunk : _rle._chunks) {
                pos += chunk._repetitions;
                if(id >= pos) {
                    it++;
                    continue;
                }
                return it;
            } 
            return _rle._chunks.end();
        }

        //! change the value of a position in the rle data
        //! search overhead
        void set(const size_t id, const base_t &val) {
            size_t chunk_end = 0;
            auto it = _rle._chunks.begin();
            for (auto &chunk : _rle._chunks) {
                chunk_end += chunk._repetitions;
                it++;
                if(id >= chunk_end) {
                    continue;
                }

                const size_t back_reps = (chunk_end-id) - 1;
                const size_t frnt_reps = (chunk._repetitions - (chunk_end-id));
                const size_t mid_reps = 1;

                // change or delete current chunk
                chunk._repetitions = frnt_reps;
                it = _rle._chunks.insert(it, {back_reps, chunk._value});
                _rle._chunks.insert(it, {mid_reps, val});
                break;
            } 
        }

        //! rle -> array
        std::vector<base_t> decode() const {
            std::vector<base_t> buf;
            for (const auto &iter : _rle._chunks) {
                for(int i = 0; i < iter._repetitions; i++) {
                    buf.push_back(iter._value);
                }
            }
            return buf;
        }

        //! array -> rle
        void encode(const std::vector<base_t> &buf) {
            for (size_t x = 0; x < buf.size(); x++) {
                add(buf.at(x));
            }
        }

        // ctor section
        rle() = default;
        rle(const std::vector<base_t> &buf) { encode(buf); }
    };
};