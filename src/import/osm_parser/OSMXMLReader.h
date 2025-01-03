#ifndef MAPSHADERS_OSMXMLREADER_H
#define MAPSHADERS_OSMXMLREADER_H
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <string>

#include "../../util/GlobalRequirements.h"
#include "../util/ParserOutputFile.h"
#include "godot_cpp/variant/packed_int64_array.hpp"

using namespace godot;

namespace osmxmlreader {

template <typename Visitor>
class Parser {
 public:
  Parser(const String& filename, Visitor& visitor)
      : visitor(visitor), parser(memnew(XMLParser)) {
    parser.ptr()->open(filename);
  }
  void parse() {
    int print_line = 0;
    while (parser->read() == 0) {
      XMLParser::NodeType cur_node_type = parser->get_node_type();
      bool xml_node_finished = false;
      if (print_line + 100000 < parser->get_current_line()) {
        print_line = parser->get_current_line();
        // std::cout << parser->get_current_line() << std::endl;
      }
      if (cur_node_type == XMLParser::NodeType::NODE_ELEMENT)
        xml_node_finished = parse_xml_node();
      else if (cur_node_type == XMLParser::NodeType::NODE_ELEMENT_END)
        xml_node_finished = true;

      if (xml_node_finished) parse_xml_node_end();
    }
  }

  bool parse_xml_node() {
    const bool pop_node = parser->is_empty();
    xml_stack.push_back(Dictionary());
    Dictionary d = static_cast<Dictionary>(xml_stack[xml_stack.size() - 1]);
    const String element_type = parser->get_node_name();
    d["element_type"] = element_type;

    if (parser->has_attribute("id")) {
      d["id"] = parser->get_named_attribute_value("id").to_int();
    }

    if (element_type == "bounds")
      visitor.bounds_callback(
          parser->get_named_attribute_value("minlon").to_float(),
          parser->get_named_attribute_value("maxlon").to_float(),
          parser->get_named_attribute_value("maxlat").to_float(),
          parser->get_named_attribute_value("minlat").to_float());
    else if (element_type == "node") {
      d["pos_geo_lon"] = parser->get_named_attribute_value("lon").to_float();
      d["pos_geo_lat"] = parser->get_named_attribute_value("lat").to_float();
    } else if (element_type == "way")
      d["nodes"] = Array();
    else if (element_type == "relation" || element_type == "area") {
      d["members"] = Array();
    } else if (element_type == "member") {
      Dictionary member_d;
      member_d["id"] = parser->get_named_attribute_value("ref").to_int();
      member_d["role"] = parser->get_named_attribute_value("role");
      member_d["type"] = parser->get_named_attribute_value("type");
      static_cast<Array>(
          static_cast<Dictionary>(xml_stack[xml_stack.size() - 2])["members"])
          .push_back(member_d);
    } else if (element_type == "nd") {
      static_cast<Array>(
          static_cast<Dictionary>(xml_stack[xml_stack.size() - 2])["nodes"])
          .push_back(parser->get_named_attribute_value("ref").to_int());
    } else if (element_type == "tag") {
      static_cast<Dictionary>(
          xml_stack[xml_stack.size() - 2])[parser->get_named_attribute_value(
          "k")] = parser->get_named_attribute_value("v");
    }

    return pop_node;
  }

  void parse_xml_node_end() {
    Dictionary item = xml_stack.back();
    const String element_type = item["element_type"].stringify();

    if (element_type != "node" && element_type != "way" &&
        element_type != "relation") {
      xml_stack.pop_back();
      return;
    }

    if (element_type == "node") {
      visitor.node_callback(item);
    } else if (element_type == "way") {
      visitor.way_callback(item);
    } else if (element_type == "relation") {
      visitor.relation_callback(item);
    }

    xml_stack.pop_back();
  }

 private:
  Visitor& visitor;
  godot::Ref<godot::XMLParser> parser;
  godot::TypedArray<godot::Dictionary> xml_stack;
  godot::Ref<GlobalRequirements> reqs;
};

template <typename Visitor>
void read_osm_xml(const String& filename, Visitor& visitor) {
  Parser<Visitor> p(filename, visitor);
  p.parse();
}
}  // namespace osmxmlreader
#endif  // MAPSHADERS_OSMXMLREADER_H