#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace xl {

struct sheet;
struct row;
struct cell;
struct column;

struct workbook {
    std::string app_name;
    std::vector<sheet> sheets;
};

struct sheet {
    std::string name;
    std::vector<row> rows;
    std::map<int, column> columns;
};

struct column {
    int width = 0;
};

struct row {
    std::vector<cell> cells;
    int height = 0;
};

struct cell_picture {
    std::string ext;
    std::vector<std::byte> blob;
};

using cell_data = std::variant<std::monostate, bool, float, std::string, cell_picture>;

struct alignment {
    std::string horizontal;
    std::string vertical;
};

struct xf {
    xl::alignment alignment;
};

struct cell {
    cell_data data;
    xl::xf xf;
    cell() {}
    cell(cell const&) = default;
    cell(cell&&) = default;
    cell(cell_data const& v)
        : data{v}
    {
    }
    cell(cell_data&& v)
        : data{std::move(v)}
    {
    }
};

} // namespace xl