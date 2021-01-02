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
#include "open3d/core/SizeVector.h"

namespace open3d {
namespace core {

inline char BigEndianTest() {
    int x = 1;
    return (((char*)&x)[0]) ? '<' : '>';
}

inline char MapType(const std::type_info& t) {
    if (t == typeid(float)) return 'f';
    if (t == typeid(double)) return 'f';
    if (t == typeid(long double)) return 'f';

    if (t == typeid(int)) return 'i';
    if (t == typeid(char)) return 'i';
    if (t == typeid(short)) return 'i';
    if (t == typeid(long)) return 'i';
    if (t == typeid(long long)) return 'i';

    if (t == typeid(unsigned char)) return 'u';
    if (t == typeid(unsigned short)) return 'u';
    if (t == typeid(unsigned long)) return 'u';
    if (t == typeid(unsigned long long)) return 'u';
    if (t == typeid(unsigned int)) return 'u';

    if (t == typeid(bool)) return 'b';

    if (t == typeid(std::complex<float>)) return 'c';
    if (t == typeid(std::complex<double>)) return 'c';
    if (t == typeid(std::complex<long double>)) return 'c';

    return '?';
}

template <typename T>
std::vector<char> CreateNpyHeader(const std::vector<size_t>& shape);
void ParseNpyHeader(FILE* fp,
                    char& type,
                    size_t& word_size,
                    std::vector<size_t>& shape,
                    bool& fortran_order);

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
        num_elements_ = 1;
        for (size_t i = 0; i < shape_.size(); i++) {
            num_elements_ *= shape_[i];
        }
        data_holder =
                std::make_shared<std::vector<char>>(num_elements_ * word_size_);
    }

    NpyArray()
        : shape_(0), word_size_(0), fortran_order_(0), num_elements_(0) {}

    template <typename T>
    T* GetDataPtr() {
        return reinterpret_cast<T*>(&(*data_holder)[0]);
    }

    template <typename T>
    const T* GetDataPtr() const {
        return reinterpret_cast<T*>(&(*data_holder)[0]);
    }

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

    SizeVector GetShape() const {
        return SizeVector(shape_.begin(), shape_.end());
    }

    bool GetFortranOrder() const { return fortran_order_; }

    size_t NumBytes() const { return data_holder->size(); }

    static NpyArray Load(const std::string& file_name) {
        FILE* fp = fopen(file_name.c_str(), "rb");
        if (!fp) {
            utility::LogError("NpyLoad: Unable to open file {}.", file_name);
        }
        std::vector<size_t> shape;
        size_t word_size;
        bool fortran_order;
        char type;
        ParseNpyHeader(fp, type, word_size, shape, fortran_order);
        NpyArray arr(shape, type, word_size, fortran_order);
        size_t nread = fread(arr.GetDataPtr<char>(), 1, arr.NumBytes(), fp);
        if (nread != arr.NumBytes()) {
            utility::LogError("LoadTheNpyFile: failed fread");
        }
        fclose(fp);
        return arr;
    }

private:
    std::shared_ptr<std::vector<char>> data_holder;
    std::vector<size_t> shape_;
    char type_;
    size_t word_size_;
    bool fortran_order_;
    size_t num_elements_;
};

template <typename T>
inline std::vector<char>& operator+=(std::vector<char>& lhs, const T rhs) {
    // Write in little endian
    for (size_t byte = 0; byte < sizeof(T); byte++) {
        char val = *((char*)&rhs + byte);
        lhs.push_back(val);
    }
    return lhs;
}

template <>
inline std::vector<char>& operator+=(std::vector<char>& lhs,
                                     const std::string rhs) {
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
    return lhs;
}

template <>
inline std::vector<char>& operator+=(std::vector<char>& lhs, const char* rhs) {
    // write in little endian
    size_t len = strlen(rhs);
    lhs.reserve(len);
    for (size_t byte = 0; byte < len; byte++) {
        lhs.push_back(rhs[byte]);
    }
    return lhs;
}

template <typename T>
void NpySave(std::string fname,
             const T* data,
             const std::vector<size_t> shape) {
    FILE* fp = fopen(fname.c_str(), "wb");
    std::vector<char> header = CreateNpyHeader<T>(shape);
    size_t nels = std::accumulate(shape.begin(), shape.end(), 1,
                                  std::multiplies<size_t>());

    fseek(fp, 0, SEEK_SET);
    fwrite(&header[0], sizeof(char), header.size(), fp);
    fseek(fp, 0, SEEK_END);
    fwrite(data, sizeof(T), nels, fp);
    fclose(fp);
}

template <typename T>
void NpySave(std::string fname,
             const std::vector<T> data,
             std::string mode = "w") {
    std::vector<size_t> shape;
    shape.push_back(data.size());
    NpySave(fname, &data[0], shape, mode);
}

template <typename T>
std::vector<char> CreateNpyHeader(const std::vector<size_t>& shape) {
    std::vector<char> dict;
    dict += "{'descr': '";
    dict += BigEndianTest();
    dict += MapType(typeid(T));
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
