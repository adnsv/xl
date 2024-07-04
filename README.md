# xl
A simple C++ header-only library for generating Excel (.xlsx) files

## Usage

1. Populate workbook model (see `xl/model.hpp`)

```c++
#include <xl/model.cpp>

auto wb = xl::workbook{};
wb.app_name = "My App";
auto& sh = wb.sheets.emplace_back("sheet1");

auto& r = sh.rows.emplace_back();
r.cells.emplace_back("A1");
r.cells.emplace_back("B1");
r.cells.emplace_back("C1");
```

2. Prepare internal file structure (see `xl/writer.hpp`)

```c++
#include <xl/writer.cpp>

auto w = xl::writer();
w.write(wb);
```

3. Pack populated content into a zip file with `.xlsx` extension (`xl/pack.hpp`)

```c++
#include <xl/pack.cpp>
std::vector<std::byte> blob;
blob = xl::pack(w.files);
// write the blob to 
```