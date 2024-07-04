#pragma once

#include <charconv>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <xl/fnv64.hpp>
#include <xl/model.hpp>
#include <xl/xml.hpp>

namespace xl {

struct writer {
    struct rel_info {
        std::string type;
        std::string target;
    };

    struct media_info {
        std::string name;
        std::vector<std::byte> const& blob;
        std::size_t iid;
        std::string rid;
    };

    std::map<std::string, std::string> files;

    std::map<std::string, rel_info> global_rels;    // maps id to absolute path
    std::map<std::string, rel_info> workbook_rels;  // maps id to absolute paths
    std::map<std::string, rel_info> rich_data_rels; // maps id to absolute paths

    std::map<std::string, std::string> default_content_types; // maps[path extension]->content-type
    std::map<std::string, std::string> part_content_types;    // maps [path partname]->content-type

    std::vector<std::string> shared_strings;
    std::map<std::string, std::size_t> shared_string_map;

    std::vector<media_info> media;
    std::map<std::string, std::size_t> media_map; // maps media name to media index

    std::vector<xl::xf> cell_xfs;

    int last_global_id = 0;
    int last_workbook_id = 0;
    int last_rich_data_id = 0;

    writer();

    void write(workbook const& wb);

    auto shared_string(std::string const&) -> std::size_t;
    auto next_global_id() -> int;
    auto next_workbook_id() -> int;
    auto next_rich_data_id() -> int;
    auto rel_id(int id) -> std::string;
    void write_core_properties();
    void write_extended_properties(std::string const& appname);
    void write_workbook(workbook const&);
    void write_sheet(sheet const& sheet, std::string const& sheet_rid);
    void write_shared_strings();
    void write_styles();
    void write_media();
    void write_metadata();
    void write_rich_value_rel();
    void write_rich_value_structure();
    void write_rich_value_data();
    void write_rich_value_types();
    void write_rels(std::string const& path, std::map<std::string, rel_info> const& rels);
    void write_content_types();
};

namespace detail {

inline auto is_empty(xl::alignment const& v) -> bool
{
    return v.horizontal.empty() && v.vertical.empty();
}

inline auto is_empty(xl::xf const& v) -> bool
{
    if (!is_empty(v.alignment))
        return false;

    return true;
}

inline auto operator==(xl::alignment const& a, xl::alignment const& b) -> bool
{
    return a.horizontal == b.horizontal && a.vertical == b.vertical;
}

template <typename T> auto same(std::optional<T> const& a, std::optional<T> const& b) -> bool
{
    if (!a && !b)
        return true;
    if (a && b)
        return *a == *b;
    return false;
}

inline auto operator==(xl::xf const& a, xl::xf const& b) -> bool
{
    if (a.alignment != b.alignment)
        return false;

    return true;
}

inline auto find(std::vector<xl::xf> const& xfs, xl::xf const& v) -> std::size_t
{
    for (std::size_t i = 0; i < xfs.size(); ++i)
        if (xfs[i] == v)
            return i;
    return std::size_t(-1);
}

} // namespace detail

inline writer::writer()
{
    default_content_types["xml"] = "application/xml";
    default_content_types["rels"] = "application/vnd.openxmlformats-package.relationships+xml";
}

inline void writer::write(workbook const& wb)
{
    write_workbook(wb);
    if (!media.empty()) {
        write_media();
        write_rich_value_rel();
        write_rels("/xl/richData/_rels/richValueRel.xml.rels", rich_data_rels);
        write_rich_value_structure();
        write_rich_value_data();
        write_metadata();
    }
    write_core_properties();
    write_extended_properties(wb.app_name);
    if (!shared_strings.empty())
        write_shared_strings();

    if (!cell_xfs.empty())
        write_styles();

    write_rels("/xl/_rels/workbook.xml.rels", workbook_rels);
    write_rels("/_rels/.rels", global_rels);

    write_content_types();
}

inline auto writer::rel_id(int id) -> std::string
{
    return std::string("rId") + std::to_string(id);
}

inline void writer::write_core_properties()
{
    auto rid = rel_id(next_global_id());

    auto const relpath = std::string("docProps/core.xml");
    auto const abspath = std::string("/") + relpath;

    part_content_types[abspath] = "application/vnd.openxmlformats-package.core-properties+xml";
    global_rels[rid] = rel_info{
        .type =
            "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("cp:coreProperties",
        {
            {"xmlns:cp", "http://schemas.openxmlformats.org/package/2006/metadata/core-properties"},
            {"xmlns:dc", "http://purl.org/dc/elements/1.1/"},
            {"xmlns:dcterms", "http://purl.org/dc/terms/"},
            {"xmlns:dcmitype", "http://purl.org/dc/dcmitype/"},
            {"xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance"},
        },
        {});

    files[abspath] = buf;
}

inline void writer::write_extended_properties(std::string const& appname)
{
    auto rid = rel_id(next_global_id());

    auto const relpath = std::string("docProps/app.xml");
    auto const abspath = std::string("/") + relpath;

    part_content_types[abspath] =
        "application/vnd.openxmlformats-officedocument.extended-properties+xml";
    global_rels[rid] = rel_info{
        .type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
                "extended-properties",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("Properties",
        {
            {"xmlns", "http://schemas.openxmlformats.org/officeDocument/2006/extended-properties"},
            {"xmlns:vt", "http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes"},
        },
        [&](xl::xw& w) {
            if (!appname.empty())
                w.node("Application", {}, [&](xl::xw& w) { w.scramble(appname); });
        });

    files[abspath] = buf;
}

inline void writer::write_workbook(workbook const& wb)
{
    auto rid = rel_id(next_global_id());

    auto const relpath = std::string("xl/workbook.xml");
    auto const abspath = std::string("/") + relpath;

    part_content_types[abspath] =
        "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml";
    global_rels[rid] = rel_info{
        .type =
            "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("workbook",
        {
            {"xmlns", "http://schemas.openxmlformats.org/spreadsheetml/2006/main"},
            {"xmlns:r", "http://schemas.openxmlformats.org/officeDocument/2006/relationships"},
        },
        [&](xw& w) {
            w.node("sheets", {}, [&](xw& w) {
                for (auto const& sheet : wb.sheets) {
                    auto const sheet_id = next_workbook_id();
                    auto const sheet_rid = rel_id(sheet_id);
                    w.node("sheet",
                        {
                            {"name", sheet.name},
                            {"sheetId", std::to_string(sheet_id)},
                            {"r:id", sheet_rid},
                        },
                        {});
                    write_sheet(sheet, sheet_rid);
                }
            });
        });

    files[abspath] = buf;
}

inline auto col_number_as_letters(int n) -> std::string
{
    auto s = std::string{};
    while (n > 0) {
        auto c = char((n - 1) % 26 + 65);
        s = std::string{c} + s;
        n = (n - 1) / 26;
    }
    return s;
}

inline void writer::write_sheet(sheet const& sh, std::string const& rid)
{
    auto const relpath = std::string{"worksheets/"} + sh.name + ".xml";
    auto const abspath = std::string{"/xl/"} + relpath;

    part_content_types[abspath] =
        "application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml";
    workbook_rels[rid] = rel_info{
        .type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("worksheet",
        {
            {"xmlns", "http://schemas.openxmlformats.org/spreadsheetml/2006/main"},
            {"xmlns:r", "http://schemas.openxmlformats.org/officeDocument/2006/relationships"},
        },
        [&](xl::xw& w) {
            if (!sh.columns.empty())
                w.node("cols", {}, [&](xl::xw& w) {
                    for (auto const& [n, c] : sh.columns) {
                        auto attrs = std::map<std::string, std::string>{
                            {"min", std::to_string(n)}, {"max", std::to_string(n)}};
                        if (c.width > 0) {
                            attrs["width"] = std::to_string(c.width);
                            attrs["customWidth"] = "1";
                        }
                        w.node("col", attrs, {});
                    }
                });

            w.node("sheetData", {}, [&](xl::xw& w) {
                auto row_number = 0;
                for (auto const& row : sh.rows) {
                    ++row_number;
                    auto attrs =
                        std::map<std::string, std::string>{{"r", std::to_string(row_number)}};
                    if (row.height > 0) {
                        attrs["ht"] = std::to_string(row.height);
                        attrs["customHeight"] = "1";
                    }
                    w.node("row", attrs, [&](xl::xw& w) {
                        auto col_number = 0;
                        for (auto const& cell : row.cells) {
                            ++col_number;

                            auto t = std::string{};
                            auto v = std::string{};
                            auto vm = std::string{};

                            if (auto d = std::get_if<bool>(&cell.data)) {
                                t = "b";
                                v = *d ? "1" : "0";
                            }
                            else if (auto d = std::get_if<float>(&cell.data)) {
                                t = "n";
                                char bb[64];
                                auto [p, _] = std::to_chars(bb, bb + 64, *d);
                                v = {bb, p};
                            }
                            else if (auto d = std::get_if<std::string>(&cell.data)) {
                                auto i = shared_string(*d);
                                t = "s";
                                v = std::to_string(i);
                            }
                            else if (auto d = std::get_if<cell_picture>(&cell.data)) {
                                auto ext = d->ext;
                                if (ext == ".jpeg" || ext == ".jpg") {
                                    ext = ".jpeg";
                                    default_content_types["jpeg"] = "image/jpeg";
                                }
                                else if (d->ext == ".png")
                                    default_content_types["png"] = "image/png";
                                else
                                    throw std::runtime_error(
                                        std::string{"unsupported image extension: "} + d->ext);

                                t = "e";
                                v = "#VALUE!";

                                auto const hash = fnv64(d->blob.data(), d->blob.size());
                                char bb[64];
                                auto [p, _] = std::to_chars(bb, bb + 64, hash, 16);
                                auto n = std::string{bb, p} + ext;
                                if (auto m = media_map.find(n); m != media_map.end()) {
                                    vm = std::to_string(m->second);
                                }
                                else {
                                    auto media_id = next_rich_data_id();
                                    auto iid = media.size();
                                    media.push_back(media_info{
                                        .name = n,
                                        .blob = d->blob,
                                        .iid = iid,
                                        .rid = rel_id(media_id),
                                    });
                                    media_map[n] = iid;
                                    vm = std::to_string(iid + 1);
                                }
                            }

                            attrs.clear();
                            attrs["r"] =
                                col_number_as_letters(col_number) + std::to_string(row_number);
                            if (!t.empty())
                                attrs["t"] = t;
                            if (!vm.empty())
                                attrs["vm"] = vm;

                            if (!detail::is_empty(cell.xf)) {
                                auto idx = detail::find(this->cell_xfs, cell.xf);
                                if (idx >= cell_xfs.size()) {
                                    idx = cell_xfs.size();
                                    cell_xfs.push_back(cell.xf);
                                }
                                attrs["s"] = std::to_string(idx + 1);
                            }

                            if (!v.empty())
                                w.node("c", attrs, [&](xl::xw& w) {
                                    w.node("v", {}, [&](xl::xw& w) { w.scramble(v); });
                                });
                        }
                    });
                }
            });
        });

    files[abspath] = buf;
}

inline void writer::write_shared_strings()
{
    auto rid = rel_id(next_workbook_id());

    auto const relpath = std::string("sharedStrings.xml");
    auto const abspath = std::string("/xl/") + relpath;

    part_content_types[abspath] =
        "application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml";
    workbook_rels[rid] = rel_info{
        .type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("sst",
        {
            {"xmlns", "http://schemas.openxmlformats.org/spreadsheetml/2006/main"},
            {"count", std::to_string(shared_strings.size())},
            {"uniqueCount", std::to_string(shared_strings.size())},
        },
        [&](xl::xw& w) {
            for (auto const& s : shared_strings)
                w.node("si", {},
                    [&](xl::xw& w) { w.node("t", {}, [&](xl::xw& w) { w.scramble(s); }); });
        });

    files[abspath] = buf;
}

inline void writer::write_styles()
{
    auto rid = rel_id(next_workbook_id());

    auto const relpath = std::string("styles.xml");
    auto const abspath = std::string("/xl/") + relpath;

    part_content_types[abspath] =
        "application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml";
    workbook_rels[rid] = rel_info{
        .type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("styleSheet",
        {
            {"xmlns", "http://schemas.openxmlformats.org/spreadsheetml/2006/main"},
        },
        [&](xl::xw& w) {
            if (!cell_xfs.empty()) {

                w.node("fonts", {{"count", "1"}},
                    [&](xl::xw& w) { w.node("font", {}, [&](xl::xw& w) {}); });

                w.node("fills", {{"count", "1"}}, [&](xl::xw& w) {
                    w.node("fill", {},
                        [&](xl::xw& w) { w.node("patternFill", {{"patternType", "none"}}, {}); });
                });

                w.node("borders", {{"count", "1"}}, [&](xl::xw& w) {
                    w.node("border", {}, [&](xl::xw& w) {
                        w.node("left", {}, {});
                        w.node("right", {}, {});
                        w.node("top", {}, {});
                        w.node("bottom", {}, {});
                        w.node("diagonal", {}, {});
                    });
                });

                w.node("cellStyleXfs", {{"count", "1"}}, [&](xl::xw& w) {
                    w.node("xf",
                        {
                            {"numFmtId", "0"},
                            {"fontId", "0"},
                            {"fillId", "0"},
                            {"borderId", "0"},
                        },
                        {});
                });

                auto default_attrs = std::map<std::string, std::string>{
                    {"numFmtId", "0"},
                    {"fontId", "0"},
                    {"fillId", "0"},
                    {"borderId", "0"},
                    {"xfId", "0"},
                };

                w.node("cellXfs", {{"count", std::to_string(cell_xfs.size() + 1)}}, [&](xl::xw& w) {
                    w.node("xf", default_attrs, {});
                    for (auto const& xf : cell_xfs) {
                        auto attrs = default_attrs;
                        if (!detail::is_empty(xf.alignment)) {
                            attrs["applyAlignment"] = "1";
                        }

                        w.node("xf", attrs, [&](xl::xw& w) {
                            if (!detail::is_empty(xf.alignment)) {
                                auto aa = std::map<std::string, std::string>{};
                                if (!xf.alignment.horizontal.empty())
                                    aa["horizontal"] = xf.alignment.horizontal;
                                if (!xf.alignment.vertical.empty())
                                    aa["vertical"] = xf.alignment.vertical;
                                w.node("alignment", aa, {});
                            }
                        });
                    }
                });
            }
        });

    files[abspath] = buf;
}

inline void writer::write_media()
{
    if (media.empty())
        return;

    for (auto const& m : media) {
        auto const fn = std::string{"/xl/media/"} + m.name;
        files[fn] = std::string{reinterpret_cast<char const*>(m.blob.data()), m.blob.size()};
        rich_data_rels[m.rid] = rel_info{
            .type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image",
            .target = std::string{"../media/"} + m.name,
        };
    }
}

inline void writer::write_metadata()
{
    auto rid = rel_id(next_workbook_id());

    auto const relpath = std::string("metadata.xml");
    auto const abspath = std::string("/xl/") + relpath;

    part_content_types[abspath] =
        "application/vnd.openxmlformats-officedocument.spreadsheetml.sheetMetadata+xml";
    workbook_rels[rid] = rel_info{
        .type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/sheetMetadata",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("metadata",
        {
            {"xmlns", "http://schemas.openxmlformats.org/spreadsheetml/2006/main"},
            {"xmlns:xlrd", "http://schemas.microsoft.com/office/spreadsheetml/2017/richdata"},
        },
        [&](xl::xw& w) {
            w.node("metadataTypes", {{"count", "1"}}, [](xl::xw& w) {
                auto attrs = std::map<std::string, std::string>{
                    {"name", "XLRICHVALUE"},
                    {"minSupportedVersion", "120000"},
                    {"copy", "1"},
                    {"pasteAll", "1"},
                    {"pasteValues", "1"},
                    {"merge", "1"},
                    {"splitFirst", "1"},
                    {"rowColShift", "1"},
                    {"clearFormats", "1"},
                    {"clearComments", "1"},
                    {"assign", "1"},
                    {"coerce", "1"},

                };

                w.node("metadataType",
                    {
                        {"name", "XLRICHVALUE"},
                        {"minSupportedVersion", "120000"},
                        {"copy", "1"},
                        {"pasteAll", "1"},
                        {"pasteValues", "1"},
                        {"merge", "1"},
                        {"splitFirst", "1"},
                        {"rowColShift", "1"},
                        {"clearFormats", "1"},
                        {"clearComments", "1"},
                        {"assign", "1"},
                        {"coerce", "1"},

                    },
                    {});
            });

            w.node("futureMetadata",
                {{"name", "XLRICHVALUE"}, {"count", std::to_string(media.size())}}, [&](xl::xw& w) {
                    for (auto const& m : media)
                        w.node("bk", {}, [&](xl::xw& w) {
                            w.node("extLst", {}, [&](xl::xw& w) {
                                w.node("ext", {{"uri", "{3e2802c4-a4d2-4d8b-9148-e3be6c30e623}"}},
                                    [&](xl::xw& w) {
                                        w.node("xlrd:rvb", {{"i", std::to_string(m.iid)}}, {});
                                    });
                            });
                        });
                });

            w.node("valueMetadata", {{"count", std::to_string(media.size())}}, [&](xl::xw& w) {
                for (auto const& m : media)
                    w.node("bk", {}, [&](xl::xw& w) {
                        w.node("rc", {{"t", "1"}, {"v", std::to_string(m.iid)}}, {});
                    });
            });
        });

    files[abspath] = buf;
}

void writer::write_rich_value_rel()
{
    auto rid = rel_id(next_workbook_id());

    auto const relpath = std::string("richData/richValueRel.xml");
    auto const abspath = std::string("/xl/") + relpath;

    part_content_types[abspath] = "application/vnd.ms-excel.richvaluerel+xml";
    workbook_rels[rid] = rel_info{
        .type = "http://schemas.microsoft.com/office/2022/10/relationships/richValueRel",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("richValueRels",
        {
            {"xmlns", "http://schemas.microsoft.com/office/spreadsheetml/2022/richvaluerel"},
            {"xmlns:r", "http://schemas.openxmlformats.org/officeDocument/2006/relationships"},
        },
        [&](xl::xw& w) {
            for (auto const& m : media)
                w.node("rel", {{"r:id", m.rid}}, {});
        });

    files[abspath] = buf;
}

void writer::write_rich_value_structure()
{
    auto rid = rel_id(next_workbook_id());

    auto const relpath = std::string("richData/rdrichvaluestructure.xml");
    auto const abspath = std::string("/xl/") + relpath;

    part_content_types[abspath] = "application/vnd.ms-excel.rdrichvaluestructure+xml";
    workbook_rels[rid] = rel_info{
        .type = "http://schemas.microsoft.com/office/2017/06/relationships/rdRichValueStructure",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("rvStructures",
        {
            {"xmlns", "http://schemas.microsoft.com/office/spreadsheetml/2017/richdata"},
            {"count", "1"},
        },
        [&](xl::xw& w) {
            w.node("s", {{"t", "_localImage"}}, [&](xl::xw& w) {
                w.node("k", {{"n", "_rvRel:LocalImageIdentifier"}, {"t", "i"}}, {});
                w.node("k", {{"n", "CalcOrigin"}, {"t", "i"}}, {});
            });
        });

    files[abspath] = buf;
}

void writer::write_rich_value_data()
{
    auto rid = rel_id(next_workbook_id());

    auto const relpath = std::string("richData/rdrichvalue.xml");
    auto const abspath = std::string("/xl/") + relpath;

    part_content_types[abspath] = "application/vnd.ms-excel.rdrichvalue+xml";
    workbook_rels[rid] = rel_info{
        .type = "http://schemas.microsoft.com/office/2017/06/relationships/rdRichValue",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("rvData",
        {
            {"xmlns", "http://schemas.microsoft.com/office/spreadsheetml/2017/richdata"},
            {"count", std::to_string(media.size())},
        },
        [&](xl::xw& w) {
            for (auto const& m : media)
                w.node("rv", {{"s", "0"}}, [&](xl::xw& w) {
                    w.node("v", {}, [&](xl::xw& w) { w.put(std::to_string(m.iid)); });
                    w.node("v", {}, [&](xl::xw& w) { w.put("5"); });
                });
        });

    files[abspath] = buf;
}

void writer::write_rich_value_types()
{
    auto rid = rel_id(next_workbook_id());

    auto const relpath = std::string("richData/rdRichValueTypes.xml");
    auto const abspath = std::string("/xl/") + relpath;

    part_content_types[abspath] = "application/vnd.ms-excel.rdrichvaluetypes+xml";
    workbook_rels[rid] = rel_info{
        .type = "http://schemas.microsoft.com/office/2017/06/relationships/rdRichValueTypes",
        .target = relpath,
    };

    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("rvTypesInfo",
        {
            {"xmlns", "http://schemas.microsoft.com/office/spreadsheetml/2017/richdata2"},
            {"xmlns:mc", "http://schemas.openxmlformats.org/markup-compatibility/2006"},
            {"xmlns:x", "http://schemas.openxmlformats.org/spreadsheetml/2006/main"},
            {"mc:Ignorable", "x"},
        },
        [&](xl::xw& w) {
            w.node("global", {}, [&](xl::xw& w) {
                w.node("key", {{"name", "_Self"}}, [&](xl::xw& w) {
                    w.node("flag", {{"name", "ExcludeFromFile"}, {"value", "1"}}, {});
                    w.node("flag", {{"name", "ExcludeFromCalcComparison"}, {"value", "1"}}, {});
                });

                auto mk_field = [&](std::string const& n) {
                    w.node("key", {{"name", "v"}},
                        [&](xl::xw& w) { w.node("flag", {{"name", n}, {"value", "1"}}, {}); });
                };

                mk_field("_DisplayString");
                mk_field("_Flags");
                mk_field("_Format");
                mk_field("_SubLabel");
                mk_field("_Attribution");
                mk_field("_Icon");
                mk_field("_Display");
                mk_field("_CanonicalPropertyNames");
                mk_field("_ClassificationId");
            });
        });

    files[abspath] = buf;
}

void writer::write_rels(std::string const& path, std::map<std::string, rel_info> const& rels)
{
    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("Relationships",
        {
            {"xmlns", "http://schemas.openxmlformats.org/package/2006/relationships"},
        },
        [&](xl::xw& w) {
            for (auto const& [rid, info] : rels)
                w.node("Relationship", {{"Id", rid}, {"Type", info.type}, {"Target", info.target}},
                    {});
        });

    files[path] = buf;
}

void writer::write_content_types()
{
    auto buf = std::string{};
    auto w = xw{buf};
    w.write_decl();
    w.node("Types",
        {
            {"xmlns", "http://schemas.openxmlformats.org/package/2006/content-types"},
        },
        [&](xl::xw& w) {
            for (auto const& [ext, ctype] : default_content_types)
                w.node("Default", {{"Extension", ext}, {"ContentType", ctype}}, {});
            for (auto const& [abspath, ctype] : part_content_types)
                w.node("Override", {{"PartName", abspath}, {"ContentType", ctype}}, {});
        });

    files["/[Content_Types].xml"] = buf;
}

inline auto writer::shared_string(std::string const& v) -> std::size_t
{
    if (auto it = shared_string_map.find(v); it != shared_string_map.end())
        return it->second;
    auto n = shared_strings.size();
    shared_strings.push_back(v);
    shared_string_map[v] = n;
    return n;
}

inline auto writer::next_global_id() -> int
{
    ++last_global_id;
    return last_global_id;
}

inline auto writer::next_workbook_id() -> int
{
    ++last_workbook_id;
    return last_workbook_id;
}

inline auto writer::next_rich_data_id() -> int
{
    ++last_rich_data_id;
    return last_rich_data_id;
}

} // namespace xl