/***********************************************************************/
/** @file LinKNX.hpp
 * Header file for volkszaehler.org API calls
 *
 *  @author Andreas Frisch
 *  @date   2018-12-07
 *  @email  fraxinas@dreambox.guru
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 *
 * (C) Fraunhofer ITWM
 **/
/*---------------------------------------------------------------------*/

/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LinKNX_hpp_
#define _LinKNX_hpp_

#include <ApiIF.hpp>
#include <Options.hpp>
#include <api/CurlIF.hpp>
#include <api/CurlResponse.hpp>
#include <Reading.hpp>

namespace vz {
	namespace api {

		class LinKNX : public ApiIF {
		public:
			typedef vz::shared_ptr<LinKNX> Ptr;

			LinKNX(Channel::Ptr ch, std::list<Option> options);
			~LinKNX();
	
			void send();

			void register_device();
			
			const std::string middleware() const { return _host; }

		private:
			int interval() const    { return _interval; }
			time_t first_ts() const { return _first_ts; }

		private:
			std::string _host;       /**< host of LinKNX Server */
            int _port;               /**< port of LinKNX Server */
			std::string _knx_group;  /**< KNX group ID */
			int _interval;           /**< time between 2 logmessages (sec.) */
            int _previous_val;       /**< previous value, only update if changed */

			time_t _first_ts;
			long _first_counter;
			long _last_counter;
	
		}; //class LinKNX
	
	} // namespace api
} // namespace vz
#endif /* _LinKNX_hpp_ */
