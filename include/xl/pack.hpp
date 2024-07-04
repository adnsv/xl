#pragma once

#include <xl-miniz.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace xl {

template <typename T>
    requires(std::is_trivial_v<T> && sizeof(T) == 1)
inline void pack(std::vector<T>& out, std::map<std::string, std::string> const& content)
{
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if (!mz_zip_writer_init_heap_v2(&archive, 0, 0, 0))
        throw std::runtime_error("failed to initialize in-memory archive");

    for (auto const& [name, blob] : content) {
        auto fn = name;
        if (fn.starts_with('/'))
            fn = fn.substr(1);

        if (!mz_zip_writer_add_mem(
                &archive, fn.c_str(), blob.data(), blob.size(), -1)) {
            mz_zip_writer_end(&archive);
            throw std::runtime_error("failed to add file to zip: " + fn);
        }
    }

    void* buffer;
    std::size_t size;
    if (!mz_zip_writer_finalize_heap_archive(&archive, &buffer, &size)) {
        mz_zip_writer_end(&archive);
        throw std::runtime_error("failed to finalize in-memory zip archive");
    }

    out.insert(
        out.end(), reinterpret_cast<T const*>(buffer), reinterpret_cast<T const*>(buffer) + size);

    mz_free(buffer);

    mz_zip_writer_end(&archive);
}

} // namespace xl