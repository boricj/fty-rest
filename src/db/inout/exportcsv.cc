/*
Copyright (C) 2014-2015 Eaton

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundatino; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*! \file   exportcsv.cc
    \brief  Export assets via csv
    \author Michal Vyskocil <MichalVyskocil@Eaton.com>
*/

#include <iostream>
#include <algorithm>
#include <vector>
#include <functional>

#include <cxxtools/csvserializer.h>
#include <tntdb/row.h>
#include <tntdb/transaction.h>

#include "db/assets.h"
#include <fty_common.h>
#include "shared/utilspp.h"

namespace persist {

// names of element from v_web_asset_element - they'll be printed as the first ones
static std::vector<std::string> ASSET_ELEMENT_KEYTAGS{
    "id", "name", "type", "sub_type", "location", "status", "priority", "asset_tag"};

// get all keytags available in the system and update the s argument
// + remove the duplicate names from v_web_asset_element/t_bios_asset_element table
//   so you can't export ext/name even if it's in the database
static int
s_update_keytags(
        tntdb::Connection& conn,
        const std::vector<std::string>& aek,
        std::vector<std::string>& s) {

    std::function<void(const tntdb::Row&)> \
        foo = [&s, &aek](const tntdb::Row& r)
        {
            std::string keytag;
            r["keytag"].get(keytag);
            if (std::find(aek.cbegin(), aek.cend(), keytag) != aek.end())
                return;
            if (std::find(s.cbegin(), s.cend(), keytag) == s.end())
                s.push_back(keytag);
        };

    int rv = select_ext_rw_attributes_keytags(
            conn,
            foo);
    return rv;
}

// using callbacks in cycle with maximum possible items might be difficult, so simply generate vector
// for power links and print it inside the cycle
//
// at the same time I don't think this is general enough to be in src/db, so static functions here
typedef std::vector<std::tuple<std::string, std::string, std::string>> power_links_t;
static int
s_power_links(
        tntdb::Connection& conn,
        a_elmnt_id_t id,
        power_links_t& out)
{
    row_cb_f foo = \
        [&out](const tntdb::Row& r)
        {
            std::string src_name{""};
            std::string src_out{""};
            std::string dest_in{""};
            r["src_name"].get(src_name);
            r["src_out"].get(src_out);
            r["dest_in"].get(dest_in);
            out.push_back(std::make_tuple(
                src_name,
                src_out,
                dest_in
            ));
        };
    return select_v_web_asset_power_link_src_byId(
            conn,
            id,
            foo);
}

// helper class to assist with serialization line by line
class LineCsvSerializer {
    public:
        explicit LineCsvSerializer(std::ostream& out):
            _cs{out, NULL},
            _buf{}
        {}

        void add(const std::string& s) {
            _buf.push_back(s);
        }

        void add(const uint32_t i) {
            return add(std::to_string(i));
        }

        void serialize() {
            std::vector<std::vector<std::string>> aux{};
            aux.push_back(_buf);
            _cs.serialize(aux);
            _buf.clear();
        }

    protected:
        cxxtools::CsvSerializer _cs;
        std::vector<std::string> _buf;
};

void
    export_asset_csv
        (std::ostream& out,
         bool generate_bom
        )
{
    // 0.) tntdb connection
    tntdb::Connection conn;
    std::string msg{"no connection to database"};
    try{
        conn = tntdb::connectCached(url);
    }
    catch(...)
    {
        log_error("%s", msg.c_str());
        LOG_END;
        throw std::runtime_error(msg.c_str());
    }
    tntdb::Transaction transaction{conn, true};

    if (generate_bom)
        out << "\xef\xbb\xbf";

    LineCsvSerializer lcs{out};

    // TODO: move somewhere else
    std::vector<std::string> KEYTAGS = {
        "description", "ip.1", "company", "site_name", "region", "country", "address",
        "contact_name", "contact_email", "contact_phone", "u_size",
        "manufacturer", "model", "serial_no", "runtime", "installation_date",
        "maintenance_date", "maintenance_due",
        "location_u_pos", "location_w_pos", "end_warranty_date", "hostname.1", "http_link.1"
    };

    uint32_t max_power_links = max_number_of_power_links(conn);
    if (max_power_links <= 0)
        max_power_links = 1;
    uint32_t max_groups = max_number_of_asset_groups(conn);
    if (max_groups <= 0)
        max_groups = 1;

    // put all remaining keys from the database
    int rv = s_update_keytags(conn, ASSET_ELEMENT_KEYTAGS, KEYTAGS);
    if (rv != 0)
        throw std::runtime_error(msg.c_str());

    // 1 print the first row with names
    // 1.1      names from asset element table itself
    for (const auto& k : ASSET_ELEMENT_KEYTAGS) {
        if (k == "id")
            continue;       //ugly but works
        lcs.add(k);
    }

    // 1.2      print power links
    for (uint32_t i = 0; i != max_power_links; i++) {
        std::string si = std::to_string(i+1);
        lcs.add("power_source."   + si);
        lcs.add("power_plug_src." + si);
        lcs.add("power_input."    + si);
    }

    // 1.3      print extended attributes
    for (const auto& k : KEYTAGS) {
        lcs.add(k);
    }

    // 1.4      print groups
    for (uint32_t i = 0; i != max_groups; i++) {
        std::string si = std::to_string(i+1);
        lcs.add("group."   + si);
    }

    lcs.add("id");
    lcs.serialize();

    // 2. FOR EACH ROW from v_web_asset_element / t_bios_asset_element do ...
    std::function<void(const tntdb::Row&)>
        process_v_web_asset_element_row \
        = [&conn, &lcs, &KEYTAGS, max_power_links, max_groups, &msg](const tntdb::Row& r)
    {
        a_elmnt_id_t id_num = 0;
        std::string id;
        r["id"].get(id_num);
        std::pair<std::string,std::string> asset_names = persist::id_to_name_ext_name (id_num);
        if (asset_names.first.empty () && asset_names.second.empty ())
            throw std::runtime_error(msg.c_str());
        id = asset_names.first;

        a_elmnt_id_t id_parent_num = 0;
        std::string location;
        r["id_parent"].get(id_parent_num);
        location = persist::id_to_name_ext_name (id_parent_num).second;

        // 2.1      select all extended attributes
        std::map <std::string, std::pair<std::string, bool> > ext_attrs;
        int rv = select_ext_attributes(conn, id_num, ext_attrs);
        if (rv != 0)
            throw std::runtime_error(msg.c_str());

        // 2.3 links
        power_links_t power_links;
        rv = s_power_links(conn, id_num, power_links);
        if (rv != 0)
            throw std::runtime_error(msg.c_str());
        // 3.4 groups
        std::vector<std::string> groups;
        rv = select_group_names(conn, id_num, groups);
        if (rv != 0)
            throw std::runtime_error(msg.c_str());

        // 2.5      PRINT IT
        // 2.5.1    things from asset element table itself
        // ORDER of fields added to the lcs IS SIGNIFICANT
        std::string type_name;
        {
        lcs.add(asset_names.second);

        r["type_name"].get(type_name);
        lcs.add(type_name);

        std::string subtype_name = "";
        // subtype for groups is stored as ext/type
        if (type_name == "group") {
            if (ext_attrs.count("type") == 1) {
                subtype_name = ext_attrs["type"].first;
                ext_attrs.erase("type");
            }
        }
        else {
            r["subtype_name"].get(subtype_name);
        }
        if ( subtype_name == "N_A" )
            subtype_name = "";
        lcs.add (utils::strip (subtype_name));

        lcs.add(location);

        std::string status;
        r["status"].get(status);
        lcs.add(status);

        uint32_t priority;
        r["priority"].get(priority);
        lcs.add("P" + std::to_string(priority));

        std::string asset_tag;
        r["asset_tag"].get(asset_tag);
        lcs.add(asset_tag);
        }

        // 2.5.2        power location
        for (uint32_t i = 0; i != max_power_links; i++) {
            std::string source{""};
            std::string plug_src{""};
            std::string input{""};

            if (i >= power_links.size()) {
                //nothing here, exists only for consistency reasons
            }
            else {
                int rv = persist::name_to_extname (std::get<0>(power_links[i]), source);
                if (rv != 0)
                    throw std::runtime_error(msg.c_str());
                plug_src = std::get<1>(power_links[i]);
                input    = std::get<2>(power_links[i]);
            }
            lcs.add(source);
            lcs.add(plug_src);
            lcs.add(input);
        }

        // convert necessary ids to names, for now just logical_asset
        {
            auto it = ext_attrs.find ("logical_asset");
            if (it != ext_attrs.end ()) {
                std::string extname;
                int rv = persist::name_to_extname (it->second.first, extname);
                if (rv != 0)
                    throw std::runtime_error(msg.c_str());
                ext_attrs ["logical_asset"] = make_pair (extname, it->second.second);
            }
        }
        // 2.5.3        read-write (!read_only) extended attributes
        for (const auto& k : KEYTAGS) {
            if (ext_attrs.count(k) == 1 &&
                !ext_attrs[k].second) {
                lcs.add(ext_attrs[k].first);
            } else {
                lcs.add("");
            }
        }

        // 2.5.4        groups
        for (uint32_t i = 0; i != max_groups; i++) {
            if (i >= groups.size())
                lcs.add("");
            else {
                std::string extname;
                int rv = persist::name_to_extname (groups[i], extname);
                if (rv != 0)
                    throw std::runtime_error(msg.c_str());
                lcs.add(extname);
            }
        }

        lcs.add(id);
        lcs.serialize();

    };

    rv = select_asset_element_all(
            conn,
            process_v_web_asset_element_row);
    if (rv != 0)
        throw std::runtime_error(msg.c_str());
    transaction.commit();
}

} // namespace persist
