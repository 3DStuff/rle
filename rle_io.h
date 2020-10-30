#pragma once

#include "rle.h"

#include <string>
#include <fstream>


template<class base_t>
class rle_io {
private:
    rle<base_t> _rle;
    std::vector<size_t> _meta;

public:
    rle_io() = default;
    rle_io(rle<base_t> &rle, const std::vector<size_t> &meta = {}) 
    : _rle(rle), _meta(meta) 
    {}

    //! return meta information: e.g. size of defined x,y,z, ... dimensions
    const std::vector<size_t> &meta() const {
        return _meta;
    }

    //! return simple and plain rle data
    const rle<base_t> &get() const {
        return _rle;
    }

    void clear() {
        _meta.clear();
        _rle.clear();
    }

    void from_file(const std::string &file) {
        auto f = std::ifstream(file, std::ofstream::binary);
        clear();

        // first meta info
        size_t nr_meta;
        f.read((char*)&nr_meta, sizeof(size_t));
        for(size_t i = 0; i < nr_meta; i++) {
            size_t meta;
            f.read((char*)&meta, sizeof(rle_chunk<size_t>));
            _meta.push_back(meta);
        }

        // second rle data
        size_t nr_chunks;
        f.read((char*)&nr_chunks, sizeof(size_t));
        f.read((char*)&_rle.data()._uncompressed_size, sizeof(size_t));
        for(size_t i = 0; i < nr_chunks; i++) {
            rle_chunk<base_t> chunk;
            f.read((char*)&chunk, sizeof(rle_chunk<base_t>));
            _rle.data()._chunks.push_back(chunk);
        }
    }

    void to_file(const std::string &file) {
        auto f = std::ofstream(file, std::ofstream::binary);

        // first meta info
        size_t nr_meta = _meta.size();
        f.write((char*)&nr_meta, sizeof(size_t));
        for(const size_t &m : _meta) {
            f.write((char*)&m, sizeof(size_t));
        }

        // second rle data
        size_t chunks = _rle.data()._chunks.size();
        f.write((char*)&chunks, sizeof(size_t));
        f.write((char*)&_rle.data()._uncompressed_size, sizeof(size_t));
        for(const auto &chunk : _rle.data()._chunks) {
            f.write((char*)&chunk, sizeof(rle_chunk<base_t>));
        }
    }
};
