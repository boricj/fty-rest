/*
 *
 * Copyright (C) 2015 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*!
 * \file test-utils-web.cc
 * \author Karol Hrdina <KarolHrdina@Eaton.com>
 * \brief Not yet documented file
 */
#include <catch.hpp>
#include <czmq.h>
#include <string>
#include <cstdlib>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/jsondeserializer.h>
#include <limits.h>
#include <fty_common.h>

TEST_CASE ("utils::json::escape","[utils::math::dtos][json][escape]")
{

    // utils::json::escape (<first>) should equal <second>
    std::vector <std::pair <std::string, std::string>> tests {
        {"'jednoduche ' uvozovky'",                                     "'jednoduche ' uvozovky'"},
        {"'jednoduche '' uvozovky'",                                    "'jednoduche '' uvozovky'"},
        {"dvojite \" uvozovky",                                         R"(dvojite \" uvozovky)"},
        {"dvojite \\\" uvozovky",                                       R"(dvojite \\\" uvozovky)"},
        {"dvojite \\\\\" uvozovky",                                     R"(dvojite \\\\\" uvozovky)"},
        {"dvojite \\\\\\\" uvozovky",                                   R"(dvojite \\\\\\\" uvozovky)"},
        {"dvojite \\\\\\\\\" uvozovky",                                 R"(dvojite \\\\\\\\\" uvozovky)"},
        {"'",                                                           R"(')"},
        {"\"",                                                          R"(\")"},
        {"'\"",                                                         R"('\")"},
        {"\"\"",                                                        R"(\"\")"},
        {"\"\"\"",                                                      R"(\"\"\")"},
        {"\\\"\"\"",                                                    R"(\\\"\"\")"},
        {"\"\\\"\"",                                                    R"(\"\\\"\")"},
        {"\"\"\\\"",                                                    R"(\"\"\\\")"},
        {"\\\"\\\"\\\"",                                                R"(\\\"\\\"\\\")"},
        {"\" dvojite \\\\\" uvozovky \"",                               R"(\" dvojite \\\\\" uvozovky \")"},
        {"dvojite \"\\\"\" uvozovky",                                   R"(dvojite \"\\\"\" uvozovky)"},
        {"dvojite \\\\\"\\\\\"\\\\\" uvozovky",                         R"(dvojite \\\\\"\\\\\"\\\\\" uvozovky)"},

        {"\b",                                                          R"(\\b)"},
        {"\\b",                                                         R"(\\b)"},
        {"\\\b",                                                        R"(\\\\b)"},
        {"\\\\b",                                                       R"(\\\\b)"},
        {"\\\\\b",                                                      R"(\\\\\\b)"},
        {"\\\\\\b",                                                     R"(\\\\\\b)"},
        {"\\\\\\\b",                                                    R"(\\\\\\\\b)"},
        {"\\\\\\\\b",                                                   R"(\\\\\\\\b)"},
        {"\\\\\\\\\b",                                                  R"(\\\\\\\\\\b)"},

        {"\\",                                                          R"(\\)"},
        {"\\\\",                                                        R"(\\\\)"},
        {"\\\\\\",                                                      R"(\\\\\\)"},
        {"\\\\\\\\",                                                    R"(\\\\\\\\)"},
        {"\\\\\\\\\\",                                                  R"(\\\\\\\\\\)"},

        {"\uA66A",                                                      "\uA66A"},
        {"Ꙫ",                                                           "Ꙫ"},
        {"\uA66A Ꙫ",                                                    "\uA66A Ꙫ"},

        /*
        {"\\uA66A",                                                     R"(\\uA66A)"},
        {"\\Ꙫ",                                                         R"(\\\u00ea\u0099\u00aa)"},
        {"\u040A Њ",                                                    R"(\u00d0\u008a \u00d0\u008a)"},
        {"\u0002\u0005\u0018\u001B",                                    R"(\u0002\u0005\u0018\u001b)"},

        {"\\\uA66A",                                                    R"(\\\u00ea\u0099\u00aa)"},
        {"\\\\uA66A",                                                   R"(\\\\uA66A)"},
        {"\\\\\uA66A",                                                  R"(\\\\\u00ea\u0099\u00aa)"},

        {"\\\\Ꙫ",                                                       R"(\\\\\u00ea\u0099\u00aa)"},
        {"\\\\\\Ꙫ",                                                     R"(\\\\\\\u00ea\u0099\u00aa)"},
        */

        {"first second \n third\n\n \\n \\\\\n fourth",                 R"(first second \\n third\\n\\n \\n \\\\\\n fourth)"},
        /*
        {"first second \n third\n\"\n \\n \\\\\"\f\\\t\\u\u0007\\\n fourth", R"(first second \\n third\\n\"\\n \\n \\\\\"\\f\\\\t\\u\u0007\\\\n fourth)"}
        */
    };

    // a valid json { key : utils::json::escape (<string> } is constructed,
    // fed into cxxtools::JsonDeserializer (), read back and compared 

    std::vector <std::string> tests_reading{
        {"newline in \n text \n\"\n times two"},
        {"x\tx"},
        {"x\\tx"},
        {"x\\\tx"},
        {"x\\\\tx"},
        {"x\\\\\tx"},
        {"x\\\\\\tx"},
        {"x\\\\\\\tx"},
        {"x\\Ꙫ\uA66A\n \\nx"},
        {"sdf\ndfg\n\\\n\\\\\n\b\tasd \b f\\bdfg"},
        {"first second \n third\n\"\n \\n \\\\\"\f\\\t\\u\u0007\\\n fourth"}
    };

    SECTION ("Manual comparison.") {
        std::string escaped;
        for (auto const& item : tests) {
            escaped = utils::json::escape (item.first);
            CAPTURE (escaped);
            CAPTURE (item.second);
            CHECK ( escaped.compare (item.second) == 0);
        }
    }
    SECTION ("Validate whether the escaped string is a valid json using cxxtools::JsonDeserializer.") {
        for (auto const& item : tests) {
            std::string json;
            std::string escaped = utils::json::escape (item.first);

            json.assign("{ \"string\" : ").append ("\"").append (escaped).append ("\" }");

            std::stringstream input (json, std::ios_base::in);
            cxxtools::JsonDeserializer deserializer (input);
            cxxtools::SerializationInfo si;
            CHECK_NOTHROW (deserializer.deserialize (si));
        }
    }

    SECTION ("Construct json, read back, compare.") {
        for (auto const& it : tests_reading) {
            std::string json;
            json.assign("{ \"read\" : ").append ("\"").append (utils::json::escape (it)).append ("\" }");

            std::stringstream input (json, std::ios_base::in);
            cxxtools::JsonDeserializer deserializer (input);
            cxxtools::SerializationInfo si;
            CHECK_NOTHROW (deserializer.deserialize (si));
            std::string read;
            si.getMember ("read") >>= read;
/*  TODO: feel free to fix this 
            CAPTURE (read);
            CAPTURE (it);
            CHECK ( read.compare (it) == 0 );
*/            
        }   
    }
}

TEST_CASE ("utils::json::jsonify","[utils::json::jsonify][json][escape]")
{
    // 
    int var_int = -1;
    long int var_long_int = -2;
    long long int var_long_long_int = -3;
    short var_short = 4;
    int32_t var_int32_t = 6;
    int64_t var_int64_t = 7;
    byte var_byte = 8;

    unsigned int var_unsigned_int = 10;
    unsigned long int var_unsigned_long_int = 20;
    unsigned long long int var_unsigned_long_long_int = 30;
    unsigned short var_unsigned_short = 40;
    uint32_t var_uint32_t = 50;
    uint64_t var_uint64_t = 60;

    //
    const char *const_char = "*const char with a '\"' quote and newline \n '\\\"'";
    std::string str = const_char;
    std::string& str_ref = str;
    std::string* str_ptr = &str;

    //
    std::vector<std::string> vector_str{"j\nedna", "dva", "tri"};
    std::vector<int> vector_int{1, 2, 3};

    std::list<std::string> list_str{"styri", "p\"at", "sest", "sedem"};
    std::list<int> list_int{4, -5, 6, 7};

    std::string x; // temporary result placeholder
    
    SECTION ("single parameter ('inttype') invocation") {
        x = utils::json::jsonify (var_int);
        CHECK ( x.compare (std::to_string (var_int)) == 0);

        x = utils::json::jsonify (var_long_int);
        CHECK ( x.compare (std::to_string (var_long_int)) == 0);

        x = utils::json::jsonify (var_long_long_int);
        CHECK ( x.compare (std::to_string (var_long_long_int)) == 0);

        x = utils::json::jsonify (var_short);
        CHECK ( x.compare (std::to_string (var_short)) == 0);

        x = utils::json::jsonify (var_int32_t);
        CHECK ( x.compare (std::to_string (var_int32_t)) == 0);

        x = utils::json::jsonify (var_int64_t);
        CHECK ( x.compare (std::to_string (var_int64_t)) == 0);

        x = utils::json::jsonify (var_byte);
        CHECK ( x.compare (std::to_string (var_byte)) == 0);

        x = utils::json::jsonify (var_unsigned_int);
        CHECK ( x.compare (std::to_string (var_unsigned_int)) == 0);

        x = utils::json::jsonify (var_unsigned_long_int);
        CHECK ( x.compare (std::to_string (var_unsigned_long_int)) == 0);

        x = utils::json::jsonify (var_unsigned_long_long_int);
        CHECK ( x.compare (std::to_string (var_unsigned_long_long_int)) == 0);

        x = utils::json::jsonify (var_unsigned_short);
        CHECK ( x.compare (std::to_string (var_unsigned_short)) == 0);

        x = utils::json::jsonify (var_uint32_t);
        CHECK ( x.compare (std::to_string (var_uint32_t)) == 0);

        x = utils::json::jsonify (var_uint64_t);
        CHECK ( x.compare (std::to_string (var_uint64_t)) == 0);

    }

    SECTION ("single parameter ('string') invocation") {

        x = utils::json::jsonify (const_char);
        CHECK ( x.compare ("\"*const char with a '\\\"' quote and newline \\\\n '\\\\\\\"'\"") == 0);

        x = utils::json::jsonify (str);
        CHECK ( x.compare ("\"*const char with a '\\\"' quote and newline \\\\n '\\\\\\\"'\"") == 0);

        x = utils::json::jsonify (str_ref);
        CHECK ( x.compare ("\"*const char with a '\\\"' quote and newline \\\\n '\\\\\\\"'\"") == 0);

        x = utils::json::jsonify (*str_ptr);
        CHECK ( x.compare ("\"*const char with a '\\\"' quote and newline \\\\n '\\\\\\\"'\"") == 0);

    }
    
    SECTION ("single parameter ('iterable standard container') invocation") {

        x = utils::json::jsonify (vector_str);
        CHECK ( x.compare (R"([ "j\\nedna", "dva", "tri" ])") == 0);
    
        x = utils::json::jsonify (vector_int);
        CHECK ( x.compare (R"([ 1, 2, 3 ])") == 0);
    
        x = utils::json::jsonify (list_str);
        CHECK ( x.compare (R"([ "styri", "p\"at", "sest", "sedem" ])") == 0);
    
        x = utils::json::jsonify (list_int);
        CHECK ( x.compare (R"([ 4, -5, 6, 7 ])") == 0);
    
    }

    SECTION ("pairs") {
        x = utils::json::jsonify (*str_ptr, var_int64_t);
        CHECK ( x.compare (std::string("\"*const char with a '\\\"' quote and newline \\\\n '\\\\\\\"'\" : ") + std::to_string (var_int64_t)) == 0);
        
        x = utils::json::jsonify ("hey\"!\n", str_ref);
        CHECK ( x.compare ("\"hey\\\"!\\\\n\" : \"*const char with a '\\\"' quote and newline \\\\n '\\\\\\\"'\"") == 0);

        x = utils::json::jsonify (-6, -7);
        CHECK ( x.compare (R"("-6" : -7)") == 0 ); 

        x = utils::json::jsonify (var_uint64_t, str);
        CHECK ( x.compare (std::string ("\"") + std::to_string (var_uint64_t) + "\" : " + "\"*const char with a '\\\"' quote and newline \\\\n '\\\\\\\"'\"") == 0 ); 


        x = utils::json::jsonify ("test", vector_str);
        CHECK ( x.compare (std::string ("\"test\" : ").append (R"([ "j\\nedna", "dva", "tri" ])")) == 0);


        x = utils::json::jsonify (4, vector_int);
        CHECK ( x.compare (std::string ("\"4\" : ").append ("[ 1, 2, 3 ]")) == 0);
    }
}

TEST_CASE ("utils::json::create_error_json","[utils::json::create_error_json][json][escape]")
{
    std::string x, in = "One and two \nthree and \"four\".";
    std::string res = "{\n\t\"errors\": [\n\t\t{\n\t\t\t\"message\": \"One and two \\nthree and \\\"four\\\".\",\n\t\t\t\"code\": 55\n\t\t}\n\t]\n}\n";


    x = utils::json::create_error_json (in.c_str (), 55);
    CHECK ( x.compare (
"{\n\t\"errors\": [\n\t\t{\n\t\t\t\"message\": \"One and two \\\\nthree and \\\"four\\\".\",\n\t\t\t\"code\": 55\n\t\t}\n\t]\n}\n"
) == 0);

    x = utils::json::create_error_json (in, 56);
    CHECK ( x.compare (
"{\n\t\"errors\": [\n\t\t{\n\t\t\t\"message\": \"One and two \\\\nthree and \\\"four\\\".\",\n\t\t\t\"code\": 56\n\t\t}\n\t]\n}\n"
) == 0);

    std::vector <std::tuple<uint32_t, std::string, std::string>> v;
    v.push_back (std::make_tuple (1, "On\ne", ""));
    v.push_back (std::make_tuple (10, "Tw\"o", ""));

    v.clear ();
    v.push_back (std::make_tuple (47, "Received value 'abc'.", ""));
    v.push_back (std::make_tuple (47, "Received value 'def'.", ""));
    v.push_back (std::make_tuple (47, "Received value 'ghi'.", ""));
    x = utils::json::create_error_json (v);
    CAPTURE (x); 
    CHECK ( x.compare (
"{\n\t\"errors\": [\n\t\t{\n\t\t\t\"message\": \"Received value 'abc'.\",\n\t\t\t\"code\": 47\n\t\t},\n\t\t{\n\t\t\t\"message\": \"Received value 'def'.\",\n\t\t\t\"code\": 47\n\t\t},\n\t\t{\n\t\t\t\"message\": \"Received value 'ghi'.\",\n\t\t\t\"code\": 47\n\t\t}\n\t]\n}\n") == 0 );

}

TEST_CASE ("utils::string_to_element_id", "[utils]") {
    uint32_t r = 0;

    SECTION ("valid input") {
        CHECK_NOTHROW( r = utils::string_to_element_id ("12") );
        CHECK ( r == 12 );
        CHECK_NOTHROW( r = utils::string_to_element_id ("123") );
        CHECK ( r == 123 );
        CHECK_NOTHROW( r = utils::string_to_element_id ("321") );
        CHECK ( r == 321 );
        CHECK_NOTHROW( r = utils::string_to_element_id ("10000") );
        CHECK ( r == 10000 );
        CHECK_NOTHROW( r = utils::string_to_element_id ("131275768") );
        CHECK ( r == 131275768 );

        CHECK_NOTHROW ( r = utils::string_to_element_id ("1")  );
        CHECK ( r == 1 );
        CHECK_NOTHROW ( r = utils::string_to_element_id ("4294967295")  );
        CHECK ( r == 4294967295 );
    }
    SECTION ("bad input - std::out_of_range") {
        CHECK_THROWS_AS( utils::string_to_element_id ("0"), std::out_of_range );
        CHECK_THROWS_AS( utils::string_to_element_id ("-1"), std::out_of_range );
        CHECK_THROWS_AS( utils::string_to_element_id ("4294967296"), std::out_of_range );
        CHECK_THROWS_AS( utils::string_to_element_id ("-433838485"), std::out_of_range );
        CHECK_THROWS_AS( utils::string_to_element_id ("13412342949672"), std::out_of_range );
        CHECK_THROWS_AS( utils::string_to_element_id ("-4387435873868"), std::out_of_range );
        CHECK_THROWS_AS( utils::string_to_element_id ("111111111111111111111111111111111111111111111111111111111111111111111111111111111"), std::out_of_range );
        CHECK_THROWS_AS( utils::string_to_element_id ("-222222222222222222222222222222222222222222222222222222222222222222222222222222222222"), std::out_of_range );
    }
    SECTION ("bad input - std::invalid_argument") {
        CHECK_THROWS_AS( utils::string_to_element_id (""), std::invalid_argument );
        CHECK_THROWS_AS( utils::string_to_element_id ("-"), std::invalid_argument );
        CHECK_THROWS_AS( utils::string_to_element_id ("x"), std::invalid_argument );
        CHECK_THROWS_AS( utils::string_to_element_id ("12s3"), std::invalid_argument );
        CHECK_THROWS_AS( utils::string_to_element_id ("s333"), std::invalid_argument );
        CHECK_THROWS_AS( utils::string_to_element_id ("asf;dguh;8y;34yt83y[Y['8\u6AA6sg "), std::invalid_argument );
    }
}

TEST_CASE ("utils::config") {

    std::mutex test_mutex {};
    std::lock_guard<std::mutex> test_lock {test_mutex};

    std::string JSON =
        "{"
        "\"BIOS_SMTP_VERIFY_CA\" : true,"
        "\"BIOS_SMTP_SERVER\" : \"string\","
        "\"BIOS_SMTP_PORT\" : 42,"
        "\"BIOS_SNMP_COMMUNITY_NAME\" : [\"foo\", \"bar\"],"
        "\"config\" : {\"key\" : \"old_array\", \"value\" : [\"old_value1\", \"old_value2\"]}"
        "}";

    std::stringstream input {JSON};
    cxxtools::JsonDeserializer deserializer (input);
    cxxtools::SerializationInfo request_doc;
    deserializer.deserialize (request_doc);

    std::map <std::string, zconfig_t*> roots;
    utils::config::json2zpl (roots, request_doc, test_lock);

    zconfig_t *config = roots [utils::config::get_path ("BIOS_SMTP_VERIFY_CA")];
    CHECK (streq (zconfig_get (config, "smtp/verify_ca", "false"), "true"));

    config = roots [utils::config::get_path ("BIOS_SNMP_COMMUNITY_NAME")];
    CHECK (streq (zconfig_get (config, "snmp/community/0", "NULL"), "foo"));
    CHECK (streq (zconfig_get (config, "snmp/community/1", "NULL"), "bar"));

    // legacy_path
    config = roots [utils::config::get_path ("old_array")];
    CHECK (!zconfig_get (config, "old_array", NULL));
    CHECK (zconfig_locate (config, "old_array") != NULL);

    CHECK (streq (zconfig_get (config, "old_array/0", "NULL"), "old_value1"));
    CHECK (streq (zconfig_get (config, "old_array/1", "NULL"), "old_value2"));

    // change value
    std::string JSON2 =
        "{"
        "\"BIOS_SMTP_VERIFY_CA\" : false,"
        "\"BIOS_SNMP_COMMUNITY_NAME\" : [\"ham\", \"spam\"],"
        "\"config\" : {\"key\" : \"old_array\", \"value\" : [\"new_value42\", \"new_value44\"]}"
        "}";

    std::stringstream input2 {JSON2};
    cxxtools::JsonDeserializer deserializer2 (input2);
    cxxtools::SerializationInfo request_doc2;
    deserializer2.deserialize (request_doc2);

    utils::config::json2zpl (roots, request_doc2, test_lock);

    config = roots [utils::config::get_path ("BIOS_SMTP_VERIFY_CA")];
    CHECK (streq (zconfig_get (config, "smtp/verify_ca", "true"), "false"));
    config = roots [utils::config::get_path ("BIOS_SNMP_COMMUNITY_NAME")];
    CHECK (streq (zconfig_get (config, "snmp/community/0", "NULL"), "ham"));
    CHECK (streq (zconfig_get (config, "snmp/community/1", "NULL"), "spam"));

    // legacy_path
    config = roots [utils::config::get_path ("old_array")];
    CHECK (!zconfig_get (config, "old_array", NULL));
    CHECK (zconfig_locate (config, "old_array") != NULL);

    CHECK (streq (zconfig_get (config, "old_array/0", "NULL"), "new_value42"));
    CHECK (streq (zconfig_get (config, "old_array/1", "NULL"), "new_value44"));

    //legacy_path single value
    std::string JSON3 =
        "{ \"config\" : "
        "{\"key\" : \"BIOS_SMTP_VERIFY_CA\", \"value\" : false}"
        "}";

    std::stringstream input3 {JSON3};
    cxxtools::JsonDeserializer deserializer3 (input3);
    cxxtools::SerializationInfo request_doc3;
    deserializer3.deserialize (request_doc3);

    utils::config::json2zpl (roots, request_doc3, test_lock);

    config = roots [utils::config::get_path ("BIOS_SMTP_VERIFY_CA")];
    CHECK (streq (zconfig_get (config, "smtp/verify_ca", "true"), "false"));

    // test less values in array
    std::string JSON4 =
        "{"
        "\"BIOS_SNMP_COMMUNITY_NAME\" : [\"eaton\"]"
        "}";
    std::stringstream input4 {JSON4};
    cxxtools::JsonDeserializer deserializer4 (input4);
    cxxtools::SerializationInfo request_doc4;
    deserializer4.deserialize (request_doc4);

    utils::config::json2zpl (roots, request_doc4, test_lock);

    config = roots [utils::config::get_path ("BIOS_SNMP_COMMUNITY_NAME")];
    zconfig_print (config);
    CHECK (streq (zconfig_get (config, "snmp/community/0", "NULL"), "eaton"));
    CHECK (!zconfig_get (config, "snmp/community/1", NULL));
    CHECK (!zconfig_get (config, "snmp/community/2", NULL));

    zconfig_destroy (&config);
}
