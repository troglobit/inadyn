/* Error code definitions
 *
 * Copyright (C) 2003-2004  Narcis Ilisei <inarcis2002@hotpop.com>
 * Copyright (C) 2010-2014  Joachim Nilsson <troglobit@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, visit the Free Software Foundation
 * website at http://www.gnu.org/licenses/gpl-2.0.html or write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "../include/error.h"
#include <stdlib.h>

typedef struct {
	int rc;
	const char *p_name;
} ERROR_NAME;

static const ERROR_NAME global_error_table[] = {
	{ RC_OK,                          "RC_OK"                        },
	{ RC_ERROR,                       "RC_ERROR"                     },
	{ RC_INVALID_POINTER,             "RC_INVALID_POINTER"           },
	{ RC_OUT_OF_MEMORY,               "RC_OUT_OF_MEMORY"             },
	{ RC_OUT_BUFFER_OVERFLOW,         "RC_OUT_BUFFER_OVERFLOW"       },
	{ RC_PIDFILE_EXISTS_ALREADY,      "RC_PIDFILE_EXISTS_ALREADY"    },

	{ RC_IP_SOCKET_CREATE_ERROR,      "RC_IP_SOCKET_CREATE_ERROR"    },
	{ RC_IP_BAD_PARAMETER,            "RC_IP_BAD_PARAMETER"          },
	{ RC_IP_INVALID_REMOTE_ADDR,      "RC_IP_INVALID_REMOTE_ADDR"    },
	{ RC_IP_CONNECT_FAILED,           "RC_IP_CONNECT_FAILED"         },
	{ RC_IP_SEND_ERROR,               "RC_IP_SEND_ERROR"             },
	{ RC_IP_RECV_ERROR,               "RC_IP_RECV_ERROR"             },
	{ RC_IP_OBJECT_NOT_INITIALIZED,   "RC_IP_OBJECT_NOT_INITIALIZED" },
	{ RC_IP_OS_SOCKET_INIT_FAILED,    "RC_IP_OS_SOCKET_INIT_FAILED"  },

	{ RC_TCP_OBJECT_NOT_INITIALIZED,  "RC_TCP_OBJECT_NOT_INITIALIZED" },

	{ RC_HTTP_OBJECT_NOT_INITIALIZED, "RC_HTTP_OBJECT_NOT_INITIALIZED" },
	{ RC_HTTP_BAD_PARAMETER,          "RC_HTTP_BAD_PARAMETER"        },

	{ RC_HTTPS_NO_TRUSTED_CA_STORE,   "RC_HTTPS_NO_TRUSTED_CA_STORE" },
	{ RC_HTTPS_OUT_OF_MEMORY,         "RC_HTTPS_OUT_OF_MEMORY"       },
	{ RC_HTTPS_FAILED_CONNECT,        "RC_HTTPS_FAILED_CONNECT"      },
	{ RC_HTTPS_FAILED_GETTING_CERT,   "RC_HTTPS_FAILED_GETTING_CERT" },
	{ RC_HTTPS_NO_SSL_SUPPORT,        "RC_HTTPS_NO_SSL_SUPPORT"      },
	{ RC_HTTPS_SEND_ERROR,            "RC_HTTPS_SEND_ERROR"          },
	{ RC_HTTPS_RECV_ERROR,            "RC_HTTPS_RECV_ERROR"          },
	{ RC_HTTPS_SNI_ERROR,             "RC_HTTPS_SNI_ERROR"           },
	{ RC_HTTPS_INVALID_REQUEST,       "RC_HTTPS_INVALID_REQUEST"     },

	{ RC_DDNS_BUFFER_TOO_SMALL,       "RC_DDNS_BUFFER_TOO_SMALL"     },
	{ RC_DDNS_INVALID_CHECKIP_RSP,    "RC_DDNS_INVALID_CHECKIP_RSP"  },
	{ RC_DDNS_INVALID_OPTION,         "RC_DDNS_INVALID_OPTION"       },
	{ RC_DDNS_INVAL_MISSING_PARAM,    "RC_DDNS_INVAL_MISSING_PARAM"  },
	{ RC_DDNS_RSP_NOTOK,              "RC_DDNS_RSP_NOTOK"            },
	{ RC_DDNS_RSP_RETRY_LATER,        "RC_DDNS_RSP_RETRY_LATER"      },

	{ RC_OS_FORK_FAILURE,             "RC_FORK_FAILURE"              },
	{ RC_OS_CHANGE_PERSONA_FAILURE,   "RC_OS_CHANGE_PERSONA_FAILURE" },
	{ RC_OS_INVALID_UID,              "RC_OS_INVALID_UID"            },
	{ RC_OS_INVALID_GID,              "RC_OS_INVALID_GID"            },

	{ RC_FILE_IO_OPEN_ERROR,          "RC_FILE_IO_OPEN_ERROR"        },
	{ RC_FILE_IO_READ_ERROR,          "RC_FILE_IO_READ_ERROR"        },
	{ RC_FILE_IO_OUT_OF_BUFFER,       "RC_FILE_IO_OUT_OF_BUFFER"     },

	{ RC_OK, NULL }
};

static const char *unknown_error = "Unknown error";

const char *error_str(int rc)
{
	const ERROR_NAME *it = global_error_table;

	while (it->p_name) {
		if (it->rc == rc)
			return it->p_name;
		it++;
	}

	return unknown_error;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
