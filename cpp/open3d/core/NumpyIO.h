// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#pragma once

#include <stdint.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include "open3d/core/Dtype.h"

namespace open3d {
namespace core {

class NpyArray {
public:
    NpyArray(const std::vector<size_t>& shape,
             char type,
             size_t word_size,
             bool fortran_order)
        : shape_(shape),
          type_(type),
          word_size_(word_size),
          fortran_order_(fortran_order) {
        num_vals = 1;
        for (size_t i = 0; i < shape_.size(); i++) {
            num_vals *= shape_[i];
        }
        data_holder =
                std::make_shared<std::vector<char>>(num_vals * word_size_);
    }

    NpyArray() : shape_(0), word_size_(0), fortran_order_(0), num_vals(0) {}

    template <typename T>
    T* data() {
        return reinterpret_cast<T*>(&(*data_holder)[0]);
    }

    template <typename T>
    const T* data() const {
        return reinterpret_cast<T*>(&(*data_holder)[0]);
    }

    size_t NumBytes() const { return data_holder->size(); }

    Dtype GetDtype() const {
        Dtype dtype(Dtype::DtypeCode::Undefined, 1, "undefined");
        if (type_ == 'f' && word_size_ == 4) {
            dtype = Dtype::Float32;
        } else if (type_ == 'f' && word_size_ == 8) {
            dtype = Dtype::Float64;
        } else if (type_ == 'i' && word_size_ == 4) {
            dtype = Dtype::Int32;
        } else if (type_ == 'i' && word_size_ == 8) {
            dtype = Dtype::Int64;
        } else if (type_ == 'u' && word_size_ == 1) {
            dtype = Dtype::UInt8;
        } else if (type_ == 'u' && word_size_ == 2) {
            dtype = Dtype::UInt16;
        } else if (type_ == 'b') {
            dtype = Dtype::Bool;
        }
        if (dtype.GetDtypeCode() == Dtype::DtypeCode::Undefined) {
            utility::LogError("Unsupported Numpy type {} word_size_ {}.", type_,
                              word_size_);
        }
        return dtype;
    }

    std::vector<size_t>& GetShape() { return shape_; }

    bool GetFortranOrder() const { return fortran_order_; }

private:
    std::shared_ptr<std::vector<char>> data_holder;
    std::vector<size_t> shape_;
    char type_;
    size_t word_size_;
    bool fortran_order_;
    size_t num_vals;
};

char BigEndianTest();
char map_type(const std::type_info& t);
template <typename T>
std::vector<char> create_npy_header(const std::vector<size_t>& shape);
void parse_npy_header(FILE* fp,
                      char& type,
                      size_t& word_size,
                      std::vector<size_t>& shape,
                      bool& fortran_order);
NpyArray npy_load(std::string fname);

template <typename T>
std::vector<char>& operator+=(std::vector<char>& lhs, const T rhs) {
    // Write in little endian
    for (size_t byte = 0; byte < sizeof(T); byte++) {
        char val = *((char*)&rhs + byte);
        lhs.push_back(val);
    }
    return lhs;
}

template <>
std::vector<char>& operator+=(std::vector<char>& lhs, const std::string rhs);
template <>
std::vector<char>& operator+=(std::vector<char>& lhs, const char* rhs);

template <typename T>
void npy_save(std::string fname,
              const T* data,
              const std::vector<size_t> shape,
              std::string mode = "w") {
    FILE* fp = NULL;
    std::vector<size_t>
            true_data_shape;  // if appending, the shape of existing + new data

    if (mode == "a") fp = fopen(fname.c_str(), "r+b");

    if (fp) {
        // file exists. we need to append to it. read the header, modify the
        // array size
        size_t word_size;
        bool fortran_order;
        char type;  // TODO: check type consistency.
        parse_npy_header(fp, type, word_size, true_data_shape, fortran_order);
        assert(!fortran_order);
        if (word_size != sizeof(T)) {
            std::cout << "libnpy error: " << fname << " has word size "
                      << word_size << " but npy_save appending data sized "
                      << sizeof(T) << "\n";
            assert(word_size == sizeof(T));
        }
        if (true_data_shape.size() != shape.size()) {
            std::cout << "libnpy error: npy_save attempting to append "
                         "misdimensioned data to "
                      << fname << "\n";
            assert(true_data_shape.size() != shape.size());
        }

        for (size_t i = 1; i < shape.size(); i++) {
            if (shape[i] != true_data_shape[i]) {
                std::cout << "libnpy error: npy_save attempting to append "
                             "misshaped data to "
                          << fname << "\n";
                assert(shape[i] == true_data_shape[i]);
            }
        }
        true_data_shape[0] += shape[0];
    } else {
        fp = fopen(fname.c_str(), "wb");
        true_data_shape = shape;
    }

    std::vector<char> header = create_npy_header<T>(true_data_shape);
    size_t nels = std::accumulate(shape.begin(), shape.end(), 1,
                                  std::multiplies<size_t>());

    fseek(fp, 0, SEEK_SET);
    fwrite(&header[0], sizeof(char), header.size(), fp);
    fseek(fp, 0, SEEK_END);
    fwrite(data, sizeof(T), nels, fp);
    fclose(fp);
}

template <typename T>
void npy_save(std::string fname,
              const std::vector<T> data,
              std::string mode = "w") {
    std::vector<size_t> shape;
    shape.push_back(data.size());
    npy_save(fname, &data[0], shape, mode);
}

template <typename T>
std::vector<char> create_npy_header(const std::vector<size_t>& shape) {
    std::vector<char> dict;
    dict += "{'descr': '";
    dict += BigEndianTest();
    dict += map_type(typeid(T));
    dict += std::to_string(sizeof(T));
    dict += "', 'fortran_order': False, 'shape': (";
    dict += std::to_string(shape[0]);
    for (size_t i = 1; i < shape.size(); i++) {
        dict += ", ";
        dict += std::to_string(shape[i]);
    }
    if (shape.size() == 1) dict += ",";
    dict += "), }";
    // pad with spaces so that preamble+dict is modulo 16 bytes. preamble is 10
    // bytes. dict needs to end with \n
    int remainder = 16 - (10 + dict.size()) % 16;
    dict.insert(dict.end(), remainder, ' ');
    dict.back() = '\n';

    std::vector<char> header;
    header += (char)0x93;
    header += "NUMPY";
    header += (char)0x01;  // major version of numpy format
    header += (char)0x00;  // minor version of numpy format
    header += (uint16_t)dict.size();
    header.insert(header.end(), dict.begin(), dict.end());

    return header;
}

}  // namespace core
}  // namespace open3d
