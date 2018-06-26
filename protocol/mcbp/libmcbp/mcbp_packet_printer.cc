/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <getopt.h>
#include <mcbp/mcbp.h>
#include <platform/dirutils.h>
#include <platform/memorymap.h>
#include <platform/sized_buffer.h>
#include <algorithm>
#include <iostream>

enum class Format { Raw, Gdb, Lldb };

Format parseFormat(std::string format) {
    std::transform(format.begin(), format.end(), format.begin(), toupper);
    if (format == "RAW") {
        return Format::Raw;
    }

    if (format == "GDB") {
        return Format::Gdb;
    }

    if (format == "LLDB") {
        return Format::Lldb;
    }

    throw std::invalid_argument("Unknown format: " + format);
}

int main(int argc, char** argv) {
    Format format{Format::Raw};

    static struct option longopts[] = {
            {"format", required_argument, nullptr, 'f'},
            {nullptr, 0, nullptr, 0}};

    int cmd;
    while ((cmd = getopt_long(argc, argv, "f:", longopts, nullptr)) != -1) {
        switch (cmd) {
        case 'f':
            try {
                format = parseFormat(optarg);
            } catch (const std::invalid_argument& e) {
                std::cerr << e.what() << std::endl;
                return EXIT_FAILURE;
            }
            break;
        default:
            std::cerr
                    << "Usage: " << cb::io::basename(argv[0])
                    << " [options] file1-n" << std::endl
                    << std::endl
                    << "\t--format=raw|gdb|lldb\tThe format for the input file"
                    << std::endl
                    << std::endl
                    << "For gdb the expected output would be produced by "
                       "executing: "
                    << std::endl
                    << std::endl
                    << "(gdb) x /24xb c->rcurr" << std::endl
                    << "0x7f43387d7e7a: 0x81 0x0d 0x00 0x00 0x00 0x00 0x00 0x00"
                    << std::endl
                    << "0x7f43387d7e82: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00"
                    << std::endl
                    << "0x7f43387d7e8a: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00"
                    << std::endl
                    << std::endl
                    << "For lldb the expected output would be generated by "
                       "executing: "
                    << std::endl
                    << std::endl
                    << "(lldb) x -c 32 c->rbuf" << std::endl
                    << "0x7f43387d7e7a: 81 0d 00 01 04 00 00 00 00 00 00 06 00 "
                       "00 00 06  ................"
                    << std::endl
                    << "0x7f43387d7e7a: 14 bf f4 26 8a e0 00 00 00 00 00 00 61 "
                       "61 81 0a  ................"
                    << std::endl
                    << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (optind == argc) {
        std::cerr << "No file specified" << std::endl;
        return EXIT_FAILURE;
    }

    while (optind < argc) {
        try {
            cb::byte_buffer buf;
            std::vector<uint8_t> data;

            cb::io::MemoryMappedFile map(
                    argv[optind], cb::io::MemoryMappedFile::Mode::RDONLY);
            auto payload = map.content();
            buf = {reinterpret_cast<uint8_t*>(payload.data()), payload.size()};

            switch (format) {
            case Format::Raw:
                break;
            case Format::Gdb:
                data = cb::mcbp::gdb::parseDump(buf);
                buf = {data.data(), data.size()};
                break;
            case Format::Lldb:
                data = cb::mcbp::lldb::parseDump(buf);
                buf = {data.data(), data.size()};
                break;
            }

            cb::mcbp::dumpStream(buf, std::cout);

        } catch (const std::exception& error) {
            std::cerr << error.what() << std::endl;
            return EXIT_FAILURE;
        }

        ++optind;
    }

    return EXIT_SUCCESS;
}
