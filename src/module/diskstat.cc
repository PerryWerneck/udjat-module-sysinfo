/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2021 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 #include <config.h>
 #include "private.h"
 #include <udjat/tools/disk/stat.h>
 #include <udjat/tools/intl.h>
 #include <sstream>
 #include <iomanip>
 #include <sstream>
 #include <udjat/tools/xml.h>
 #include <sys/time.h>
 #include <sstream>
 #include <udjat/moduleinfo.h>

 namespace Udjat {

	static const Udjat::ModuleInfo moduleinfo{"Get Disk Read/Write speed average"};

	static const char *labels[] = {
		N_( "Average disk speed" ),
		N_( "Read disk speed" ),
		N_( "Write disk speed" )
	};

	class SysInfo::DiskStat::Agent : public Abstract::Agent {
	private:
		unsigned short type;

		const Disk::Unit *unit= nullptr;

		Disk::Stat::Data diskstat;

		/// @brief Get stats from all physical disks.
		/*
		static Disk::Stat & load(Disk::Stat &stat) {

			for(Disk::Stat &disk : Disk::Stat::get()) {

				if(disk.physical()) {
					stat += disk;
				}

			}

			return stat;

		}
		*/

		float getAverage() const {
			return ((diskstat.read + diskstat.write) / 2) / unit->value;
		}

		float getRead() const {
			return diskstat.read / unit->value;
		}

		float getWrite() const {
			return diskstat.write / unit->value;
		}

		float getValueByType() const {

			switch(type) {
			case 0: // Average disk speed
				return getAverage();

			case 1: // Read disk speed
				return getRead();

			case 2: // Write disk speed
				return getWrite();
			}

			return 0;

		}

	public:
		Agent(const xml_node &node) : Abstract::Agent(node), type(Attribute(node,"stat-type").select("average","read","write",nullptr)) {

			Object::properties.icon = "utilities-system-monitor";
			this->unit = Udjat::Disk::Unit::get(node);

			if(!(Object::properties.label && *Object::properties.label)) {
#ifdef GETTEXT_PACKAGE
				Object::properties.label = Quark(string{dgettext(GETTEXT_PACKAGE,labels[type])} + _( " in " ) + unit->speed).c_str();
#else
				Object::properties.label = Quark(string{labels[type]} + " in " + unit->speed).c_str();
#endif // GETTEXT_PACKAGE
			}

			if(!timer()) {
				throw runtime_error("Disk stats requires an update timer");
			}

			for(Disk::Stat &disk : Disk::Stat::get()) {
				if(disk.physical()) {
					diskstat += Disk::Stat::Data(disk);
				}
			}

			/*
			load(st);
			st.reset(diskstat);
			*/

		}

		virtual ~Agent() {
		}

		Udjat::Value & get(Udjat::Value &value) const override {
			value = getValueByType();
			return value;
		}

		bool refresh() override {

			Disk::Stat::Data current;
			for(Disk::Stat &disk : Disk::Stat::get()) {
				if(disk.physical()) {
					current += Disk::Stat::Data(disk);
				}
			}

			diskstat.update(current);

#ifdef DEBUG
			cout << "Read: " << this->diskstat.read << " Write: " << this->diskstat.write << endl;
#endif // DEBUG

			return true;
		}

		std::string to_string() const noexcept override {
			std::stringstream out;
			out << std::fixed << std::setprecision(2) << getValueByType() << " " << unit->speed;
			return out.str();
		}

	};

	SysInfo::DiskStat::DiskStat() : Udjat::Factory("system-diskstat",moduleinfo) {
	}

	SysInfo::DiskStat::~DiskStat() {
	}

	std::shared_ptr<Abstract::Agent> SysInfo::DiskStat::AgentFactory(const Abstract::Object UDJAT_UNUSED(&parent), const pugi::xml_node &node) const {
		return make_shared<Agent>(node);
	}

 }
