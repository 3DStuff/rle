#pragma once

#include "rle.h"

#include <iostream>
#include <string>
#include <fstream>

namespace compress {
    template<class base_t>
    class rle_io {
    private:
        rle<base_t> _rle;
        std::vector<int> _meta;

    public:
        rle_io() = default;
        rle_io(const rle<base_t> &rle, const std::vector<int> &meta = {}) 
        : _rle(rle), _meta(meta) 
        {}

        //! return meta information: e.g. size of defined x,y,z, ... dimensions
        const std::vector<int> &meta() const {
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
            // size, dimensions ..
            size_t nr_meta;
            f.read((char*)&nr_meta, sizeof(size_t));
            for(size_t i = 0; i < nr_meta; i++) {
                int meta;
                f.read((char*)&meta, sizeof(int));
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
                _rle.compute_step();
            }
            f.close();
        }

        void to_file(const std::string &file) {
            auto f = std::ofstream(file, std::ofstream::binary);

            // first meta info
            size_t nr_meta = _meta.size();
            f.write((char*)&nr_meta, sizeof(size_t));
            for(int m : _meta) {
                f.write((char*)&m, sizeof(int));
            }

            std::cout << "Pack with: " << sizeof(rle_chunk<base_t>) << " bytes\n";

            // second rle data
            size_t chunks = _rle.data()._chunks.size();
            f.write((char*)&chunks, sizeof(size_t));
            if(chunks > 0) {
                f.write((char*)&_rle.data()._uncompressed_size, sizeof(size_t));
                f.write((char*)&_rle.data()._chunks[0], chunks * sizeof(rle_chunk<base_t>));
            }
            f.close();
        }
    };
};