#include "CoastlineParser.h"
#include <fstream>
#include <iostream>
#include <godot_cpp/classes/node.hpp>

#pragma pack(push, 1) // Ensures that the structs are packed with no padding

using namespace godot;

struct ShapefileHeader {
    int32_t fileCode;       
    int32_t unused[5];      
    int32_t fileLength;     
    int32_t version;        
    int32_t shapeType;      
    double xMin, yMin, xMax, yMax;
    double zMin, zMax, mMin, mMax;
};

struct RecordHeader {
    int32_t recordNumber;   
    int32_t contentLength;  
};

#pragma pack(pop)

void swapEndian(int32_t& value) {
    value = ((value >> 24) & 0xFF) | 
            ((value << 8) & 0xFF0000) | 
            ((value >> 8) & 0xFF00) | 
            ((value << 24) & 0xFF000000);
}

bool intersects(double xMin1, double yMin1, double xMax1, double yMax1,
                double xMin2, double yMin2, double xMax2, double yMax2) {
    return !(xMax1 < xMin2 || xMin1 > xMax2 || yMax1 < yMin2 || yMin1 > yMax2);
}

Array readShapefile(const std::string& shpFileName, const std::string& shxFileName, godot::Ref<GeoMap> geomap,
                   double queryXMin, double queryYMin, double queryXMax, double queryYMax) {
    std::ifstream shpFile(shpFileName, std::ios::binary);
    std::ifstream shxFile(shxFileName, std::ios::binary);
    godot::Array polygons;
    if (!shpFile || !shxFile) {
        std::cerr << "Unable to open files." << std::endl;
        return polygons;
    }

    ShapefileHeader header;
    shpFile.read(reinterpret_cast<char*>(&header), sizeof(header));
    swapEndian(header.fileCode);
    swapEndian(header.fileLength);

    // Read through the .shx file to find relevant records
    shxFile.seekg(100, std::ios::beg); // Skip the header
    while (shxFile) {
        int32_t offset, contentLength;
        shxFile.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        shxFile.read(reinterpret_cast<char*>(&contentLength), sizeof(contentLength));

        if (shxFile.gcount() == 0) break; // End of file

        swapEndian(offset);
        swapEndian(contentLength);

        shpFile.seekg(offset * 2, std::ios::beg); // Offsets in .shx are in 16-bit words

        RecordHeader recordHeader;
        shpFile.read(reinterpret_cast<char*>(&recordHeader), sizeof(recordHeader));
        swapEndian(recordHeader.recordNumber);
        swapEndian(recordHeader.contentLength);

        int32_t shapeType;
        shpFile.read(reinterpret_cast<char*>(&shapeType), sizeof(shapeType));

        if (shapeType == 1) { // Point
            double x, y;
            shpFile.read(reinterpret_cast<char*>(&x), sizeof(double));
            shpFile.read(reinterpret_cast<char*>(&y), sizeof(double));

            if (x >= queryXMin && x <= queryXMax && y >= queryYMin && y <= queryYMax) {
                std::cout << "Point Record #" << recordHeader.recordNumber 
                          << ": (" << x << ", " << y << ")" << std::endl;
            }
        } else if (shapeType == 5) { // Polygon (for example)
            double shapeXMin, shapeYMin, shapeXMax, shapeYMax;
            shpFile.read(reinterpret_cast<char*>(&shapeXMin), sizeof(double));
            shpFile.read(reinterpret_cast<char*>(&shapeYMin), sizeof(double));
            shpFile.read(reinterpret_cast<char*>(&shapeXMax), sizeof(double));
            shpFile.read(reinterpret_cast<char*>(&shapeYMax), sizeof(double));

            if (intersects(queryXMin, queryYMin, queryXMax, queryYMax, 
                           shapeXMin, shapeYMin, shapeXMax, shapeYMax)) {
                std::cout << "Polygon Record #" << recordHeader.recordNumber 
                          << " intersects with query box." << std::endl;
                // Read the full polygon data if needed
		
                // Number of parts and points
                int32_t numParts, numPoints;
                shpFile.read(reinterpret_cast<char*>(&numParts), sizeof(numParts));
                shpFile.read(reinterpret_cast<char*>(&numPoints), sizeof(numPoints));

                // Read parts array
                std::vector<int32_t> parts(numParts);
                shpFile.read(reinterpret_cast<char*>(parts.data()), numParts * sizeof(int32_t));

                // Read points array
                std::vector<std::pair<double, double>> points(numPoints);
                for (int i = 0; i < numPoints; ++i) {
                    double x, y;
                    shpFile.read(reinterpret_cast<char*>(&x), sizeof(double));
                    shpFile.read(reinterpret_cast<char*>(&y), sizeof(double));
                    points[i] = {x, y};
                }

                // Output the polygon data

                std::cout << "Polygon Record #" << recordHeader.recordNumber << " has " 
                        << numParts << " parts and " << numPoints << " points." << std::endl;

                Array polygon;
    
                for (int i = 0; i < numParts; ++i) {
                    int start = parts[i];
                    int end = (i == numParts - 1) ? numPoints : parts[i + 1];
                    std::cout << "  Part " << i + 1 << ":\n";

                    Array part;
                    for (int j = start; j < end; ++j) {
                        part.append(geomap->geo_to_world(GeoCoords(Longitude::degrees(points[j].first), Latitude::degrees(points[j].second))));
                    }

                    polygon.append(part);
                }

                polygons.append(polygon);
            }
        } else {
	    std::cout << "Shape type " << shapeType << " intersects with query box." << std::endl;
	}
    }

    shpFile.close();
    shxFile.close();

    return polygons;
}

void CoastlineParser::import(const godot::String& shpFilename, const godot::String& shxFilename, godot::Ref<GeoMap> geomap) {
    //auto polygons = readShapefile(shpFilename.utf8().get_data(), shxFilename.utf8().get_data(), geomap, 105, -8, 108, -5); indo cypel
    auto polygons = readShapefile(shpFilename.utf8().get_data(), shxFilename.utf8().get_data(), geomap, 0.19, 50.6, 0.21, 51);

    auto shader_nodes = this->get_shader_nodes();
    WARN_PRINT("Imported " + String::num_int64(polygons.size()) + " polygons.");
    WARN_PRINT("Shader nodes size: " + String::num_int64(shader_nodes.size()));
    for (int i = 0; i < shader_nodes.size(); i++) {
        Object::cast_to<Node>(shader_nodes[i])->call("import_polygons", polygons);
    }
}