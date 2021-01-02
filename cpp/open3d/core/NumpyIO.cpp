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

#include "open3d/core/NumpyIO.h"

#include <stdint.h>

#include <algorithm>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <regex>
#include <stdexcept>

#include "open3d/utility/Console.h"

namespace open3d {
namespace core {

char BigEndianTest() {
    int x = 1;
    return (((char *)&x)[0]) ? '<' : '>';
}

char MapType(const std::type_info &t) {
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

template <>
std::vector<char> &operator+=(std::vector<char> &lhs, const std::string rhs) {
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
    return lhs;
}

template <>
std::vector<char> &operator+=(std::vector<char> &lhs, const char *rhs) {
    // write in little endian
    size_t len = strlen(rhs);
    lhs.reserve(len);
    for (size_t byte = 0; byte < len; byte++) {
        lhs.push_back(rhs[byte]);
    }
    return lhs;
}

void ParseNpyHeader(FILE *fp,
                    char &type,
                    size_t &word_size,
                    std::vector<size_t> &shape,
                    bool &fortran_order) {
    char buffer[256];
    size_t res = fread(buffer, sizeof(char), 11, fp);
    if (res != 11) {
        utility::LogError("ParseNpyHeader: failed fread");
    }
    std::string header = fgets(buffer, 256, fp);
    assert(header[header.size() - 1] == '\n');

    size_t loc1, loc2;

    // fortran order
    loc1 = header.find("fortran_order");
    if (loc1 == std::string::npos) {
        utility::LogError(
                "ParseNpyHeader: failed to find header keyword: "
                "'fortran_order'");
    }

    loc1 += 16;
    fortran_order = (header.substr(loc1, 4) == "True" ? true : false);

    // shape
    loc1 = header.find("(");
    loc2 = header.find(")");
    if (loc1 == std::string::npos || loc2 == std::string::npos) {
        utility::LogError(
                "ParseNpyHeader: failed to find header keyword: '(' or ')'");
    }

    std::regex num_regex("[0-9][0-9]*");
    std::smatch sm;
    shape.clear();

    std::string str_shape = header.substr(loc1 + 1, loc2 - loc1 - 1);
    while (std::regex_search(str_shape, sm, num_regex)) {
        shape.push_back(std::stoi(sm[0].str()));
        str_shape = sm.suffix().str();
    }

    // endian, word size, data type
    // byte order code | stands for not applicable.
    // not sure when this applies except for byte array
    loc1 = header.find("descr");
    if (loc1 == std::string::npos) {
        utility::LogError(
                "ParseNpyHeader: failed to find header keyword: 'descr'");
    }

    loc1 += 9;
    bool littleEndian =
            (header[loc1] == '<' || header[loc1] == '|' ? true : false);
    assert(littleEndian);
    (void)littleEndian;

    type = header[loc1 + 1];
    // assert(type == MapType(T));

    std::string str_ws = header.substr(loc1 + 2);
    loc2 = str_ws.find("'");
    word_size = atoi(str_ws.substr(0, loc2).c_str());
}

NpyArray load_the_npy_file(FILE *fp) {
    std::vector<size_t> shape;
    size_t word_size;
    bool fortran_order;
    char type;
    ParseNpyHeader(fp, type, word_size, shape, fortran_order);

    NpyArray arr(shape, type, word_size, fortran_order);
    size_t nread = fread(arr.GetDataPtr<char>(), 1, arr.NumBytes(), fp);
    if (nread != arr.NumBytes()) {
        utility::LogError("load_the_npy_file: failed fread");
    }

    return arr;
}

NpyArray NpyLoad(std::string fname) {
    FILE *fp = fopen(fname.c_str(), "rb");

    if (!fp) {
        utility::LogError("NpyLoad: Unable to open file {}.", fname);
    }

    NpyArray arr = load_the_npy_file(fp);

    fclose(fp);
    return arr;
}

}  // namespace core
}  // namespace open3d
