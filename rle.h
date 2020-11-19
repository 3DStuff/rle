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
        size_t _prev_block_end; // stop id of last block

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

        // step for lookup
        size_t _step = 1;
        // lookup table
        static constexpr float _lu_perc = 0.02;
        static constexpr int _lu_size = (size_t)(1.f/_lu_perc)-1;
        std::array<size_t, _lu_size> _chunk_lut;

    protected:
        void init(const base_t &val) {
            clear();
            _rle._uncompressed_size = 1;
            _rle._chunks = {{ 1, val }};
        }

        void compute_step() {
            //step stuff
            _step = std::sqrt(_rle._chunks.size()) + 1;
            _step = _step > 192 ? 192 : _step;

            // lookup table
            const size_t num_chunks = _rle._chunks.size();
            for(int i = 0; i < _lu_size; i++) {
                _chunk_lut[i] = _rle._chunks[_lu_perc*(i+1) * num_chunks]._prev_block_end;
            } 
        }

        // add single value
        void add(const base_t &val) {
            if(_rle._chunks.empty() || _rle._uncompressed_size < 1) {
                init(val);
                compute_step();
                return;
            }

            auto back = _rle._chunks.rbegin();
            if(val == back->_value) {
                back->_repetitions++;
            }
            else {
                _rle._chunks.push_back({ 1, val, _rle._uncompressed_size });
            }
            _rle._uncompressed_size++;
            compute_step();
        }

        // add chunk
        void add(const size_t &num, const base_t &val) {
            if(_rle._chunks.empty()) {
                _rle._chunks.push_back({ num, val, _rle._uncompressed_size });
                _rle._uncompressed_size += num;
                compute_step();
                return;
            }

            auto &last = _rle._chunks.back();
            if(last._value == val) {
                last._repetitions += num;
                _rle._uncompressed_size += num;
                compute_step();
                return;
            }

            _rle._chunks.push_back({ num, val, _rle._uncompressed_size });
            _rle._uncompressed_size += num;
            compute_step();
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

        //! get iterator to a chunk containing the index
        //! aim to keep access time +/- constant
        const auto at(const size_t id) const {
            size_t adv = 0;
            size_t end = 0;

            // we are at the beginning
            // just return
            if(id < _rle._chunks[1]._prev_block_end) {
                return this->begin();
            }

            // we want to keep access time constant, 
            // without table access time increases with index
            constexpr int lu_end_id = _lu_size-1;
            for(int i = lu_end_id; i >= 0; i--) {
                if(id > _chunk_lut[i]) {
                    adv = (_lu_perc*(i+1)) * _rle._chunks.size();
                    end = _rle._chunks[adv]._prev_block_end;
                    break;
                }
            }

            // here why walk with coarse steps over the chunks
            // before using a fine search
            for(int i = adv; i < _rle._chunks.size(); i += _step) {
                if(id < _rle._chunks[i]._prev_block_end) {
                    break;
                }
                adv = i;
                end = _rle._chunks[i]._prev_block_end;
            }

            // continue with fine search
            for(int i = adv; i < _rle._chunks.size(); i++) {
                end += _rle._chunks[i]._repetitions;
                if(id < end) {
                    return (this->begin()+i);
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

        //! parallel rle -> array
        std::vector<base_t> decode() const {
            std::vector<base_t> buf(_rle._uncompressed_size, 0);
            size_t id = 0;
            for (const auto &iter : _rle._chunks) {
                for(int i = 0; i < iter._repetitions; i++) {
                    buf[id++] = iter._value;
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

        void encode(const std::vector<std::pair<size_t, base_t>> &buf) {
            for (size_t x = 0; x < buf.size(); x++) {
                const auto &p = buf[x];
                if(p.first == 0) continue;
                add(p.first, p.second);
            }
        }

        // ctor section
        rle() = default;
        rle(const std::vector<base_t> &buf) { encode(buf); }
    };
};