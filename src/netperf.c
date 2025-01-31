/*

	   Copyright (C) 1993-2011 Hewlett-Packard Company
                         ALL RIGHTS RESERVED.

  The enclosed software and documentation includes copyrighted works
  of Hewlett-Packard Co. For as long as you comply with the following
  limitations, you are hereby authorized to (i) use, reproduce, and
  modify the software and documentation, and to (ii) distribute the
  software and documentation, including modifications, for
  non-commercial purposes only.

  1.  The enclosed software and documentation is made available at no
      charge in order to advance the general development of
      high-performance networking products.

  2.  You may not delete any copyright notices contained in the
      software or documentation. All hard copies, and copies in
      source code or object code form, of the software or
      documentation (including modifications) must contain at least
      one of the copyright notices.

  3.  The enclosed software and documentation has not been subjected
      to testing and quality control and is not a Hewlett-Packard Co.
      product. At a future time, Hewlett-Packard Co. may or may not
      offer a version of the software and documentation as a product.

  4.  THE SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS".
      HEWLETT-PACKARD COMPANY DOES NOT WARRANT THAT THE USE,
      REPRODUCTION, MODIFICATION OR DISTRIBUTION OF THE SOFTWARE OR
      DOCUMENTATION WILL NOT INFRINGE A THIRD PARTY'S INTELLECTUAL
      PROPERTY RIGHTS. HP DOES NOT WARRANT THAT THE SOFTWARE OR
      DOCUMENTATION IS ERROR FREE. HP DISCLAIMS ALL WARRANTIES,
      EXPRESS AND IMPLIED, WITH REGARD TO THE SOFTWARE AND THE
      DOCUMENTATION. HP SPECIFICALLY DISCLAIMS ALL WARRANTIES OF
      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

  5.  HEWLETT-PACKARD COMPANY WILL NOT IN ANY EVENT BE LIABLE FOR ANY
      DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
      (INCLUDING LOST PROFITS) RELATED TO ANY USE, REPRODUCTION,
      MODIFICATION, OR DISTRIBUTION OF THE SOFTWARE OR DOCUMENTATION.

*/
char	netperf_id[]="\
@(#)netperf.c (c) Copyright 1993-2012 Hewlett-Packard Company. Version 2.6.0";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

/* FreeBSD doesn't like socket.h before types are set. */
#if __FreeBSD__
# include <sys/types.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include "missing\stdint.h"
#endif /* WIN32 */

#include "netsh.h"
#include "netlib.h"
#include "nettest_bsd.h"
#include "profile.h"

#ifdef WANT_UNIX
#include "nettest_unix.h"
#endif /* WANT_UNIX */

#ifdef WANT_XTI
#include "nettest_xti.h"
#endif /* WANT_XTI */

#ifdef WANT_DLPI
#include "nettest_dlpi.h"
#endif /* WANT_DLPI */

#ifdef WANT_SDP
#include "nettest_sdp.h"
#endif

/* The DNS tests have been removed from netperf2. Those wanting to do
   DNS_RR tests should use netperf4 instead. */

#ifdef DO_DNS
#error DNS tests have been removed from netperf. Use netperf4 instead
#endif /* DO_DNS */

#ifdef WANT_SCTP
#include "nettest_sctp.h"
#endif

 /* this file contains the main for the netperf program. all the other
    routines can be found in the file netsh.c */


int _cdecl
main(int argc, char *argv[])
{

#ifdef WIN32
  WSADATA	wsa_data ;

  /* Initialize the winsock lib ( version 2.2 ) */
  if ( WSAStartup(MAKEWORD(2,2), &wsa_data) == SOCKET_ERROR ){
    printf("WSAStartup() failed : %lu\n", GetLastError()) ;
    return 1 ;
  }
#endif /* WIN32 */

  netlib_init();
  /* the call to set_defaults() is gone because we can initialize in
     declarations (or is that definitions) unlike the old days */
  scan_cmd_line(argc,argv);

  /* Initialize performance counters */
  pmc_profile_setup("netperf");

  if (debug) {
    dump_globals();
    install_signal_catchers();
  }

  if (debug) {
    printf("remotehost is %s and port %s\n",host_name,test_port);
    fflush(stdout);
  }


  if (!no_control) {
    establish_control(host_name,test_port,address_family,
		      local_host_name,local_test_port,local_address_family);

    if (passphrase != NULL) {
      netperf_request.content.request_type = PASSPHRASE;
      strncpy((char *)netperf_request.content.test_specific_data,
	      passphrase,
	      sizeof(netperf_request.content.test_specific_data));
      send_request_n(0);
    }
  }

  if (strcasecmp(test_name,"TCP_STREAM") == 0) {
    send_tcp_stream(host_name);
  }
  else if (strcasecmp(test_name,"TCP_MAERTS") == 0) {
    send_tcp_maerts(host_name);
  }
  else if (strcasecmp(test_name,"TCP_MSS") == 0) {
    send_tcp_mss(host_name);
  }
#ifdef HAVE_ICSC_EXS
  else if (strcasecmp(test_name,"EXS_TCP_STREAM") == 0) {
    send_exs_tcp_stream(host_name);
  }
#endif /* HAVE_ICSC_EXS */
#ifdef HAVE_SENDFILE
  else if (strcasecmp(test_name,"TCP_SENDFILE") == 0) {
    sendfile_tcp_stream(host_name);
  }
#endif /* HAVE_SENDFILE */
  else if (strcasecmp(test_name,"TCP_RR") == 0) {
    send_tcp_rr(host_name);
  }
  else if (strcasecmp(test_name,"TCP_CRR") == 0) {
    send_tcp_conn_rr(host_name);
  }
  else if (strcasecmp(test_name,"TCP_CC") == 0) {
    send_tcp_cc(host_name);
  }
#ifdef DO_1644
  else if (strcasecmp(test_name,"TCP_TRR") == 0) {
    send_tcp_tran_rr(host_name);
  }
#endif /* DO_1644 */
#ifdef DO_NBRR
  else if (strcasecmp(test_name,"TCP_NBRR") == 0) {
    send_tcp_nbrr(host_name);
  }
#endif /* DO_NBRR */
  else if (strcasecmp(test_name,"UDP_STREAM") == 0) {
    send_udp_stream(host_name);
  }
  else if (strcasecmp(test_name,"UDP_RR") == 0) {
    send_udp_rr(host_name);
  }
  else if (strcasecmp(test_name,"LOC_CPU") == 0) {
    loc_cpu_rate();
  }
  else if (strcasecmp(test_name,"REM_CPU") == 0) {
    rem_cpu_rate();
  }
#ifdef WANT_DLPI
  else if (strcasecmp(test_name,"DLCO_RR") == 0) {
    send_dlpi_co_rr(host_name);
  }
  else if (strcasecmp(test_name,"DLCL_RR") == 0) {
    send_dlpi_cl_rr(host_name);
  }
  else if (strcasecmp(test_name,"DLCO_STREAM") == 0) {
    send_dlpi_co_stream(host_name);
  }
  else if (strcasecmp(test_name,"DLCL_STREAM") == 0) {
    send_dlpi_cl_stream(host_name);
  }
#endif /* WANT_DLPI */
#ifdef WANT_UNIX
  else if (strcasecmp(test_name,"STREAM_RR") == 0) {
    send_stream_rr(host_name);
  }
  else if (strcasecmp(test_name,"DG_RR") == 0) {
    send_dg_rr(host_name);
  }
  else if (strcasecmp(test_name,"STREAM_STREAM") == 0) {
    send_stream_stream(host_name);
  }
  else if (strcasecmp(test_name,"DG_STREAM") == 0) {
    send_dg_stream(host_name);
  }
#endif /* WANT_UNIX */
#ifdef WANT_XTI
  else if (strcasecmp(test_name,"XTI_TCP_STREAM") == 0) {
    send_xti_tcp_stream(host_name);
  }
  else if (strcasecmp(test_name,"XTI_TCP_RR") == 0) {
    send_xti_tcp_rr(host_name);
  }
  else if (strcasecmp(test_name,"XTI_UDP_STREAM") == 0) {
    send_xti_udp_stream(host_name);
  }
  else if (strcasecmp(test_name,"XTI_UDP_RR") == 0) {
    send_xti_udp_rr(host_name);
  }
#endif /* WANT_XTI */

#ifdef WANT_SCTP
  else if (strcasecmp(test_name, "SCTP_STREAM") == 0) {
    send_sctp_stream(host_name);
  }
  else if (strcasecmp(test_name, "SCTP_RR") == 0) {
    send_sctp_rr(host_name);
  }
  else if (strcasecmp(test_name, "SCTP_STREAM_MANY") == 0) {
    send_sctp_stream_1toMany(host_name);
  }
  else if (strcasecmp(test_name, "SCTP_RR_MANY") == 0) {
    send_sctp_rr_1toMany(host_name);
  }
#endif

#ifdef DO_DNS
  else if (strcasecmp(test_name,"DNS_RR") == 0) {
    fprintf(stderr,
	  "DNS tests can now be found in netperf4.\n");
    fflush(stderr);
    exit(-1);
  }
#endif /* DO_DNS */
#ifdef WANT_SDP
  else if (strcasecmp(test_name,"SDP_STREAM") == 0) {
    send_sdp_stream(host_name);
  }
  else if (strcasecmp(test_name,"SDP_MAERTS") == 0) {
    send_sdp_maerts(host_name);
  }
  else if (strcasecmp(test_name,"SDP_RR") == 0) {
    send_sdp_rr(host_name);
  }
#endif /* WANT_SDP */
#ifdef WANT_OMNI
  else if (strcasecmp(test_name,"OMNI") == 0) {
    send_omni(host_name);
  }
  else if (strcasecmp(test_name,"UUID") == 0) {
    print_uuid(host_name);
  }
#endif
  else {
    printf("The test you requested (%s) is unknown to this netperf.\n"
	   "Please verify that you have the correct test name, \n"
	   "and that test family has been compiled into this netperf.\n",
	   test_name);
    exit(1);
  }

  pmc_profile_teardown();

  if (!no_control) {
    shutdown_control();
  }

#ifdef WIN32
  /* Cleanup the winsock lib */
  WSACleanup();
#endif

  return(0);
}


