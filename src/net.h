/******************************************************************************
 * net module for ncurest                                                     *
 *                                                                            *
 * functions for networking,                                                  *
 ******************************************************************************/

#ifndef __NCUREST_NET_H__
#define __NCUREST_NET_H__

/**
 * Try to open a tcp socket connection to the given host, on the given port.
 *
 * Arguments:
 *   host  the hostname to connect to.
 *   port  the port to connect to on the host.
 *
 * Returns: a socket file descriptor if a connection was succesfully
 *          established, or -1 on error.
 */
int net_tcp_connect(const char *host, const char *port);

#endif // __NCUREST_NET_H__
