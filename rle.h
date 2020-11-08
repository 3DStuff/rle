#pragma once

#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <map>


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
        std::vector<rle_chunk<base_t>> _chunks = {};
        // original storage container size
        size_t _uncompressed_size = 0;
    };

    template<class base_t>
    class rle {
        template<class T> friend class rle_io;

        rle_meta<base_t> _rle;
        using chunk_t = rle_chunk<base_t>;

        // speeding up search a bit
        using rle_iter_t = std::vector<rle_chunk<base_t>>::const_iterator;
        std::map<size_t, size_t> _lookups;
        size_t _last_div = 0;

    protected:
        void init(const base_t &val) {
            clear();
            _rle._uncompressed_size = 1;
            _rle._chunks = {{ 1, val }};
        }

        // we can greatly reduce search time for a value
        // if we directly start from the mid of the list if possible
        void compute_lookup() {
            size_t num_chunks = _rle._chunks.size();
            size_t div = (std::sqrt((float)num_chunks) / 5) + 1;
            size_t step = num_chunks / div;

            if(div == _last_div) 
                return;

            std::vector<size_t> stop_ids;
            for(int i = step; i < num_chunks; i += step) {
                if(num_chunks - i >= step) {
                    stop_ids.push_back(i);
                }
            }

            size_t cur_id = 0;
            auto cur_itr = _rle._chunks.begin();
            for (auto itr = this->begin(); itr != this->end(); itr++) {
                for(size_t stop_id : stop_ids) {
                    const size_t dist = std::distance(this->begin(), itr);
                    if(dist == stop_id) {
                        _lookups[cur_id] = dist;
                    }
                }
                cur_id += itr->_repetitions;
            }
            _last_div = div;
        }

        void add(const base_t &val) {
            if(_rle._chunks.empty() || _rle._uncompressed_size < 1) {
                init(val);
                return;
            }

            auto back = _rle._chunks.rbegin();
            if(val == back->_value) {
                back->_repetitions++;
            }
            else {
                _rle._chunks.push_back({ 1, val });
                compute_lookup();
            }
            _rle._uncompressed_size++;
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

        const auto at(const size_t id) const {
            size_t chunk_end = 0;
            auto chunk_it = this->begin();

            auto start_it = _lookups.rbegin();
            auto end_it = _lookups.rend();
            if(id < _rle._uncompressed_size / 2) {
                auto start_it = _lookups.begin();
                auto end_it = _lookups.end();
            }

            for(; start_it != end_it; start_it++) {
                const size_t unpacked_id = start_it->first;
                const size_t iterator_dist = start_it->second;

                if(id < unpacked_id) continue;
                std::advance(chunk_it, iterator_dist);
                chunk_end = unpacked_id;
                break;
            }

            for (; chunk_it != this->end(); chunk_it++) {
                chunk_end += chunk_it->_repetitions;
                if(id >= chunk_end) {
                    continue;
                }
                return chunk_it;
            } 
            return this->end();
        }

        //! returns an iterator to an element in the rle array at a given position
        //! iterator::end() if nothing was found
        const auto operator [] (const size_t id) const {
            return at(id);
        }

        const auto rbegin() const {
            return _rle._chunks.rbegin();
        }

        const auto rend() const {
            return _rle._chunks.rend();
        }

        const auto begin() const {
            return _rle._chunks.begin();
        }

        const auto end() const {
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
            compute_lookup();
        }

        // ctor section
        rle() = default;
        rle(const std::vector<base_t> &buf) { encode(buf); }
    };
};