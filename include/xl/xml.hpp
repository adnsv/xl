#pragma once

#include <functional>
#include <map>
#include <string_view>

namespace xl {

struct xw {
    std::string& buffer;
    void put(std::string_view raw);
    void node(std::string_view tag, std::map<std::string, std::string> const& attrs,
        std::function<void(xw& w)>&& content);
    void scramble(std::string_view s, bool in_otag = true);
    void write_decl();
};

inline void xw::put(std::string_view raw) { buffer += raw; }

inline void xw::write_decl()
{
    put("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
}

inline void xw::node(std::string_view tag, std::map<std::string, std::string> const& attrs,
    std::function<void(xw& w)>&& content)
{
    put("<");
    put(tag);
    for (auto const& [k, v] : attrs) {
        put(" ");
        put(k);
        put("=\"");
        scramble(v, true);
        put("\"");
    }
    if (content) {
        put(">");
        content(*this);
        put("</");
        put(tag);
        put(">");
    }
    else {
        put("/>");
    }
}

inline void xw::scramble(std::string_view s, bool in_otag)
{
    if (s.empty())
        return;

    auto start = s.data();
    auto end = s.data() + s.size();
    auto cursor = start;

    auto replace = [&](std::string_view with) {
        put({start, size_t(cursor - start)});
        ++cursor;
        start = cursor;
        put(with);
    };

    while (cursor != end) {
        auto cp = *cursor;
        if (!in_otag && (cp == '\r' || cp == '\n')) {
            auto rn = cp == '\r' && cursor + 1 != end && cursor[1] == '\n';
            replace("\n");
            if (rn) {
                ++cursor;
                start = cursor;
            }
            continue;
        }

        switch (cp) {

        case '&':
            replace("&amp;");
            break;

        case '<':
            replace("&lt;");
            break;

        case '>':
            replace("&gt;");
            break;

        case '\'':
            if (in_otag)
                replace("&apos;");
            else
                ++cursor;
            break;

        case '"':
            if (in_otag)
                replace("&quot;");
            else
                ++cursor;
            break;

        case '\t':
        case '\r':
        case '\n':
            ++cursor;
            break;

        default:
            if (static_cast<unsigned char>(cp) <= '\x0f') {
                char buf[] = "&#0;";
                buf[2] += cp;
                if (cp >= '\x0a')
                    buf[2] += 'a' - ':';
                replace(buf);
            }
            else if (static_cast<unsigned char>(cp) <= '\x1f') {
                char buf[] = "&#10;";
                buf[3] += cp - 16;
                if (cp >= '\x1a')
                    buf[3] += 'a' - ':';
                replace(buf);
            }
            else
                ++cursor;
        }
    }
    if (cursor != start) {
        put({start, size_t(cursor - start)});
    }
}

} // namespace xl