#
# Lib/protos.spec -- master list of known UNSN protocols
# Copyright (C) 2000  Andrew Main
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

#
# Format: each protocol line is of the form
#   <protoname>: [<optionname>] [<optionname>] ...
# and specifies the valid options for protocol.  Options within each protocol
# MUST be listed in lexical order, and protocols within the file MUST be
# listed in lexical order.
#

ip: [] [protocol]
ipv4: [] [protocol]
ipv6: [] [protocol]
local: [] [type]
tcp: []
udp: []
