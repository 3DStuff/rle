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

    struct iter_info {
        size_t block_end;
        size_t iter_distance;
    };

    template<class base_t>
    class rle {
        template<class T> friend class rle_io;

        rle_meta<base_t> _rle;
        using chunk_t = rle_chunk<base_t>;

        // speeding up search a bit
        std::vector<iter_info> _lookups = {{ 0, 0 }};

        static size_t get_step(size_t size_arr, size_t divider = 5) {
            return size_arr / divider + 1;
        }

    protected:
        void init(const base_t &val) {
            clear();
            _rle._uncompressed_size = 1;
            _rle._chunks = {{ 1, val }};
        }

        // we can greatly reduce search time for a value
        // if we directly start from the mid of the list if possible
        void compute_lookup() {
            const size_t num_chunks = _rle._chunks.size();
            const size_t step = 192;
            if(2 * _lookups.size() >= num_chunks / step) {
                return;
            }
            _lookups = {{ 0, 0 }};

            size_t block_end = 0;
            for(size_t i = 0; i != _rle._chunks.size(); i++) {
                for(size_t j = step; j < num_chunks; j += step) {
                    if(i == j) {
                        _lookups.push_back({block_end, i});
                    }
                }
                block_end += _rle._chunks[i]._repetitions;
            }
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
            size_t adv = 0;
            size_t end = 0;
            if(id < _rle._uncompressed_size/2 || _lookups.size() < 128) {
                for(int i = 0; i < _lookups.size(); i++) {
                    if(id < _lookups[i].block_end) {
                        break;
                    }
                    adv = _lookups[i].iter_distance;
                    end = _lookups[i].block_end;
                }
            }
            else {
                for(int i = _lookups.size()-1; i >= 0; i--) {
                    adv = _lookups[i].iter_distance;
                    end = _lookups[i].block_end;
                    if(id > _lookups[i].block_end) {
                        break;
                    }
                }
            }
/*
            bool smaller = false;
            bool bigger = false;
            if(_lookups.size() > 1) {
                for(int i = _lookups.size()/2; i < _lookups.size() && i >= 0;) {
                    size_t loc_end = _lookups[i].block_end;
                    if(id == loc_end) {
                        adv = _lookups[i].iter_distance;
                        end = loc_end;
                        break;
                    }
                    else if(id < loc_end && !bigger) {
                        smaller = true;
                        i--;
                        continue;
                    }
                    else if(id > loc_end && !smaller) {
                        bigger = true;
                        i++;
                        continue;
                    }

                    if(smaller) {
                        adv = _lookups[i].iter_distance;
                        end = loc_end;
                        break;
                    }
                    if(bigger) {
                        adv = _lookups[i-1].iter_distance;
                        end = loc_end;
                        break;
                    }
                }
            }
*/
/*
            for(int i = 0; i < _lookups.size(); i++) {
                if(id < _lookups[i].block_end) {
                    break;
                }
                adv = _lookups[i].iter_distance;
                end = _lookups[i].block_end;
            }
*/
            auto chunk_it = this->begin();
            for(int i = adv; i < _rle._chunks.size(); i++) {
                end += _rle._chunks[i]._repetitions;
                if(id < end) {
                    return (chunk_it+i);
                }
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