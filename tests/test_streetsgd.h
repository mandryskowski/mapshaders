#ifndef TEST_STREETSGD
#define TEST_STREETSGD

#include "tests/test_macros.h"
#include "../import/GeoMap.h"
#include "../import/FileAccessMemoryResizable.h"

namespace TestStreetsGD {
    TEST_CASE ("[Modules][StreetsGD][GeoMap] Calculates geo origin correctly") {
        GeoMap map = GeoMap(GeoCoords(Longitude::degrees (99.0), Latitude::degrees (-51.0)),
                            GeoCoords(Longitude::degrees (101.0), Latitude::degrees (-49.0)));
        CHECK (map.geo_to_world (GeoCoords (Longitude::degrees (100.0), Latitude::degrees (-50.0))).is_zero_approx());
    }

    TEST_CASE ("[Modules][StreetsGD][GeoMap] Index works in general") {
        GeoMap map = GeoMap(GeoCoords(Longitude::degrees (99.0), Latitude::degrees (-51.0)),
                            GeoCoords(Longitude::degrees (101.0), Latitude::degrees (-49.0)),
                            3);
        CHECK (map.grid_index (map.geo_to_world(GeoCoords(Longitude::degrees (99.01), Latitude::degrees (-49.01)))) == 0);
        CHECK (map.grid_index (map.geo_to_world(GeoCoords(Longitude::degrees (100.0), Latitude::degrees (-49.01)))) == 1);
        CHECK (map.grid_index (map.geo_to_world(GeoCoords(Longitude::degrees (99.01), Latitude::degrees (-50.0)))) == 3);
        CHECK (map.grid_index (map.geo_to_world(GeoCoords(Longitude::degrees (100.0), Latitude::degrees (-50.0)))) == 4);
        CHECK (map.grid_index (map.geo_to_world(GeoCoords(Longitude::degrees (99.01), Latitude::degrees (-50.99)))) == 6);
    }

    TEST_CASE ("[Modules][StreetsGD][GeoMap] Index works on boundaries") {
        GeoMap map = GeoMap(GeoCoords(Longitude::degrees (99.0), Latitude::degrees (-51.0)),
                            GeoCoords(Longitude::degrees (101.0), Latitude::degrees (-49.0)),
                            3);
        CHECK (map.grid_index (map.geo_to_world(GeoCoords(Longitude::degrees (99.0), Latitude::degrees (-49.0)))) == 0);
        CHECK (map.grid_index (map.geo_to_world(GeoCoords(Longitude::degrees (101.0), Latitude::degrees (-49.0)))) == 2);
        CHECK (map.grid_index (map.geo_to_world(GeoCoords(Longitude::degrees (99.0), Latitude::degrees (-51.0)))) == 6);
        CHECK (map.grid_index (map.geo_to_world(GeoCoords(Longitude::degrees (101.0), Latitude::degrees (-51.0)))) == 8);
    }

    TEST_CASE ("[Modules][StreetsGD][FileAccessMemoryResizable] FileAccessMemoryResizable does not needlessly resize when full") {
        Ref<FileAccessMemoryResizable> fa = memnew (FileAccessMemoryResizable);
        fa->open_resizable (2);

        // store 2 arbitrary bytes
        fa->store_8(42u);
        fa->store_8(137u);

        CHECK (fa->get_length() == 2);
    }

    TEST_CASE ("[Modules][StreetsGD][FileAccessMemoryResizable] FileAccessMemoryResizable doubles in size when overflown") {
        Ref<FileAccessMemoryResizable> fa = memnew (FileAccessMemoryResizable);
        fa->open_resizable (2);

        // store 4 arbitrary bytes
        fa->store_8(64u);
        fa->store_8(205u);
        fa->store_8(3u);
        fa->store_8(16u);

        CHECK (fa->get_length() == 4);

        // check for potential corruption
        fa->seek (0);
        CHECK(fa->get_8() == 64u);
        CHECK(fa->get_8() == 205u);
        CHECK(fa->get_8() == 3u);
        CHECK(fa->get_8() == 16u);
    }

    TEST_CASE ("[Modules][StreetsGD][FileAccessMemoryResizable] FileAccessMemoryResizable resizes when storing a buffer") {
        Ref<FileAccessMemoryResizable> fa = memnew (FileAccessMemoryResizable);
        fa->open_resizable (2);

        // store 4 arbitrary bytes
        fa->store_8(51u);
        PackedByteArray arr = {65u, 74u, 89u, 96u};
        fa->store_buffer (&arr[0], arr.size());

        CHECK (fa->get_length() == 5);

        // check for potential corruption
        fa->seek (0);
        CHECK(fa->get_8() == 51u);
        for (int i = 0; i < arr.size(); i++)
            CHECK(fa->get_8() == arr[i]);
    }
}

#endif // TEST_STREETSGD