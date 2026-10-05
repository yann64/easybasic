#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <easybasic/runtime/systraylib.hpp>

using namespace easybasic::runtime;

namespace {
/// Mirrors runtime_guilib_test.cpp's own hasDisplay() exactly - each test
/// file is self-contained, the same convention every other runtime_*_test
/// file in this project already follows.
bool hasDisplay() {
    static bool checked = [] {
        int argc = 0;
        char** argv = nullptr;
        return gtk_init_check(&argc, &argv) == TRUE;
    }();
    return checked;
}

/// Mirrors runtime_guilib_test.cpp's own writeTinyBmp(), producing a 24-bit
/// BMP instead - gdk_pixbuf_new_from_file (the Image library's own
/// LoadImage) auto-detects format, so either works; a BMP needs no PNG
/// loader plugin assumption, matching this project's own existing
/// convention for a from-scratch test fixture.
std::filesystem::path writeTinyBmp() {
    std::filesystem::path path = std::filesystem::temp_directory_path() / "easybasic_systraylib_test.bmp";
    constexpr int width = 4;
    constexpr int height = 2;
    constexpr int rowBytes = width * 3;
    constexpr int pixelDataSize = rowBytes * height;
    constexpr int fileSize = 14 + 40 + pixelDataSize;
    std::vector<unsigned char> bytes(fileSize, 0);
    auto put16 = [&](std::size_t offset, std::uint16_t v) {
        bytes[offset] = static_cast<unsigned char>(v & 0xff);
        bytes[offset + 1] = static_cast<unsigned char>((v >> 8) & 0xff);
    };
    auto put32 = [&](std::size_t offset, std::uint32_t v) {
        for (int i = 0; i < 4; ++i) {
            bytes[offset + i] = static_cast<unsigned char>((v >> (8 * i)) & 0xff);
        }
    };
    bytes[0] = 'B';
    bytes[1] = 'M';
    put32(2, static_cast<std::uint32_t>(fileSize));
    put32(10, 54);
    put32(14, 40);
    put32(18, width);
    put32(22, height);
    put16(26, 1);
    put16(28, 24);
    put32(34, static_cast<std::uint32_t>(pixelDataSize));
    for (int i = 0; i < pixelDataSize; ++i) {
        bytes[54 + i] = 0x80;
    }
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return path;
}
} // namespace

TEST_CASE("CreatePopupMenu builds a usable menu without MenuTitle, unlike CreateMenu", "[runtime][systraylib]") {
    // Oracle-verified: a popup menu's own MenuItem/MenuBar calls work
    // immediately after CreatePopupMenu, with no MenuTitle needed first -
    // unlike a regular CreateMenu'd menu bar, whose own build stack is
    // left empty until MenuTitle populates it (confirmed directly against
    // the real oracle: attempting MenuItem right after plain CreateMenu,
    // with no MenuTitle, is a genuine GTK-level crash there).
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    CHECK(pbIsMenu(500) == 0);
    CHECK(pbCreatePopupMenu(500) == 1);
    CHECK(pbIsMenu(500) == 1);
    CHECK(pbMenuItem(1, PBString("About")) == 1);
    CHECK(pbMenuBar() == 1);
    CHECK(pbMenuItem(2, PBString("Quit")) == 1);
    CHECK(pbGetMenuItemText(500, 1).bytes() == "About");
    pbFreeMenu(500);
    CHECK(pbIsMenu(500) == 0);
}

TEST_CASE("CreatePopupImageMenu behaves like CreatePopupMenu", "[runtime][systraylib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    CHECK(pbCreatePopupImageMenu(501) == 1);
    CHECK(pbIsMenu(501) == 1);
    CHECK(pbMenuItem(1, PBString("Open")) == 1);
    pbFreeMenu(501);
}

TEST_CASE("AddSysTrayIcon/IsSysTrayIcon/RemoveSysTrayIcon round-trip", "[runtime][systraylib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    std::filesystem::path bmp = writeTinyBmp();
    pbLoadImage(600, PBString(bmp.string()));

    CHECK(pbIsSysTrayIcon(510) == 0);
    CHECK(pbAddSysTrayIcon(510, 0, pbImageID(600)) == 1);
    CHECK(pbIsSysTrayIcon(510) == 1);
    CHECK(pbIsSysTrayIcon(999999) == 0);

    CHECK(pbRemoveSysTrayIcon(510) == 0); // Oracle-verified: no return value - always 0.
    CHECK(pbIsSysTrayIcon(510) == 0);

    pbFreeImage(600);
    std::remove(bmp.string().c_str());
}

TEST_CASE("AddSysTrayIcon/ChangeSysTrayIcon with no image (0) are a harmless failure",
          "[runtime][systraylib]") {
    // Unlike every other image-consuming function's own "unknown handle"
    // test elsewhere in this project, this can't exercise an arbitrary
    // *wrong* nonzero value here - imageId is ImageID()'s own real pointer
    // (see pbAddSysTrayIcon's own doc comment), not a table key to miss a
    // lookup on, so only `0` ("no image given") is a safe case to test;
    // anything else nonzero would be read as a real (and here, bogus)
    // GdkPixbuf* and dereferenced.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    CHECK(pbAddSysTrayIcon(511, 0, 0) == 0);
    CHECK(pbIsSysTrayIcon(511) == 0);

    std::filesystem::path bmp = writeTinyBmp();
    pbLoadImage(607, PBString(bmp.string()));
    pbAddSysTrayIcon(518, 0, pbImageID(607));
    CHECK(pbChangeSysTrayIcon(518, 0) == 0);
    pbRemoveSysTrayIcon(518);
    pbFreeImage(607);
    std::remove(bmp.string().c_str());
}

TEST_CASE("ChangeSysTrayIcon writes a fresh icon file and removes the previous one",
          "[runtime][systraylib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    std::filesystem::path bmp = writeTinyBmp();
    pbLoadImage(601, PBString(bmp.string()));
    pbLoadImage(602, PBString(bmp.string()));
    pbAddSysTrayIcon(512, 0, pbImageID(601));

    std::filesystem::path iconDir = detail::sysTrayTable().at(512).iconDir;
    CHECK(std::filesystem::exists(iconDir / "icon0.png"));

    CHECK(pbChangeSysTrayIcon(512, pbImageID(602)) == 0); // Oracle-verified: no return value - always 0.
    CHECK(std::filesystem::exists(iconDir / "icon1.png"));
    CHECK_FALSE(std::filesystem::exists(iconDir / "icon0.png")); // Previous file cleaned up.

    pbRemoveSysTrayIcon(512);
    CHECK_FALSE(std::filesystem::exists(iconDir)); // RemoveSysTrayIcon removes the whole scratch directory.

    pbFreeImage(601);
    pbFreeImage(602);
    std::remove(bmp.string().c_str());
}

TEST_CASE("ChangeSysTrayIcon on an unknown tray id is a harmless failure", "[runtime][systraylib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    std::filesystem::path bmp = writeTinyBmp();
    pbLoadImage(603, PBString(bmp.string()));
    CHECK(pbChangeSysTrayIcon(999999, pbImageID(603)) == 0);

    pbFreeImage(603);
    std::remove(bmp.string().c_str());
}

TEST_CASE("RemoveSysTrayIcon(#PB_All) frees every remaining tray icon at once", "[runtime][systraylib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    std::filesystem::path bmp = writeTinyBmp();
    pbLoadImage(604, PBString(bmp.string()));
    pbAddSysTrayIcon(514, 0, pbImageID(604));
    pbAddSysTrayIcon(515, 0, pbImageID(604));
    pbRemoveSysTrayIcon(-1); // #PB_All
    CHECK(pbIsSysTrayIcon(514) == 0);
    CHECK(pbIsSysTrayIcon(515) == 0);
    pbFreeImage(604);
    std::remove(bmp.string().c_str());
}

TEST_CASE("SysTrayIconMenu associates a real popup menu with the indicator", "[runtime][systraylib]") {
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    std::filesystem::path bmp = writeTinyBmp();
    pbLoadImage(605, PBString(bmp.string()));
    pbAddSysTrayIcon(516, 0, pbImageID(605));
    pbCreatePopupMenu(502);
    pbMenuItem(1, PBString("Quit"));

    CHECK(pbSysTrayIconMenu(516, pbMenuID(502)) == 0); // Oracle-verified: no return value - always 0.
    GtkMenu* attached = app_indicator_get_menu(detail::sysTrayTable().at(516).indicator);
    CHECK(GTK_WIDGET(attached) == detail::menuTable().at(502));

    CHECK(pbSysTrayIconMenu(999999, pbMenuID(502)) == 0); // Unknown tray id - harmless.

    pbRemoveSysTrayIcon(516);
    pbFreeMenu(502);
    pbFreeImage(605);
    std::remove(bmp.string().c_str());
}

TEST_CASE("SysTrayIconToolTip sets the indicator's own title", "[runtime][systraylib]") {
    // AppIndicator has no hover-tooltip concept at all - app_indicator_set_title
    // is the closest approximation this maps onto, see pbSysTrayIconToolTip's
    // own doc comment for why.
    if (!hasDisplay()) {
        SKIP("no usable display available in this environment");
    }
    std::filesystem::path bmp = writeTinyBmp();
    pbLoadImage(606, PBString(bmp.string()));
    pbAddSysTrayIcon(517, 0, pbImageID(606));

    CHECK(pbSysTrayIconToolTip(517, PBString("Hello")) == 0); // Oracle-verified: no return value - always 0.
    CHECK(std::string(app_indicator_get_title(detail::sysTrayTable().at(517).indicator)) == "Hello");

    CHECK(pbSysTrayIconToolTip(999999, PBString("x")) == 0); // Unknown tray id - harmless.

    pbRemoveSysTrayIcon(517);
    pbFreeImage(606);
    std::remove(bmp.string().c_str());
}
