#
# Lib/errcmp.spec -- master list of known error numbers
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
# Error numbers are listed in order from the most basic errors to the
# most advanced.
#

# UNSNs: syntactically invalid
UNSN_EUNSNBADSYNTAX
# UNSNs: semantically invalid
UNSN_EUNSNEMPTY
UNSN_EUNSNDUPOPTION
UNSN_EUNSNPROTOUNREC
UNSN_EUNSNBADOPTION
UNSN_EUNSNBADVALUE
UNSN_EUNSNBADENCAP
# UNSNs: unsupported
UNSN_EUNSNPROTONOSUPPORT
UNSN_EUNSNLAYERNOSUPPORT
UNSN_EUNSNENCAPNOSUPPORT
UNSN_EUNSNCOMBINOSUPPORT

# addresses: semantically invalid
UNSN_EADDRUNSPEC

# IP addresses: semantically invalid
UNSN_EIPPROTOUNREC

# TCP/UDP addresses: semantically invalid
UNSN_ETCPPORTUNREC
UNSN_EUDPPORTUNREC

# IP addresses: host lookup failure
UNSN_EIPHOSTTFAIL
UNSN_EIPHOSTPFAIL
UNSN_EIPHOSTFAIL
UNSN_EIPNOHOST
UNSN_EIPHOSTNOADDR

# socket addresses: semantically invalid or unsupported
EPFNOSUPPORT
ESOCKTNOSUPPORT
EPROTOTYPE
ENOPROTOOPT
EPROTONOSUPPORT
EAFNOSUPPORT

# addresses: unusable together
UNSN_EADDRINCOMPAT

# socket addresses: unusable
EADDRNOTAVAIL
EADDRINUSE

# networking: failure to communicate
ETIMEDOUT
ENETUNREACH
ENETDOWN
EHOSTUNREACH
EHOSTDOWN
ECONNRESET
ECONNREFUSED

# files: invalid name
ENAMETOOLONG
ELOOP
# files: non-existent
ENOTDIR
ENOENT
# files: uncreatable
EISDIR
EEXIST
EMLINK
# files: wrong type
ENODEV
ENXIO
ENOTBLK
ENOTTY
ESPIPE
# files: operation not allowed
EPERM
EACCES
ENOTEMPTY
EXDEV
EFBIG
EROFS
EBUSY
ETXTBSY
