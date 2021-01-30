/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <string.h>
#include <microhttpd.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#define PORT 8888

#define BASEDIR "/var/www"
#define INDEX_HTML "index.html"
#define PNGTYPE "image/png"
#define JPGTYPE "image/jpg"
#define ICOTYPE "image/x-icon"
#define HTMLTYPE "text/html"

static int
answer_to_connection (void *cls,
		      struct MHD_Connection *connection,
                      const char *url,
		      const char *method,
                      const char *version,
		      const char *upload_data,
                      size_t *upload_data_size,
		      void **con_cls)
{
  struct MHD_Response *response;
  int fd;
  int ret;
  struct stat sbuf;
  char file_name[256] = BASEDIR;
  char* ext;
  char content_type[100] = HTMLTYPE;
  (void)cls;               /* Unused. Silent compiler warning. */
  /*(void)url;                Unused. Silent compiler warning. */
  /*(void)method;             Unused. Silent compiler warning. */
  /*(void)version;            Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */
  (void)con_cls;           /* Unused. Silent compiler warning. */

  printf ("New %s request for %s using version %s\n", method, url, version);
  
  if (0 != strcmp (method, "GET"))
    return MHD_NO;
  strcpy(file_name,strcat(file_name,url));

  if ( file_name[strlen(file_name)-1] == '/' )
    strcpy(file_name,strcat(file_name,INDEX_HTML));

  ext = &file_name[strlen(file_name)-3];

  if (0 == strcmp(ext, "png"))
    strcpy(content_type, PNGTYPE);

  if (0 == strcmp(ext, "ico"))
    strcpy(content_type, ICOTYPE);
  
  if (0 == strcmp(ext, "jpg") || 0 == strcmp(ext, "peg"))
    strcpy(content_type, JPGTYPE);

  if ( (-1 == (fd = open (file_name, O_RDONLY))) ||
       (0 != fstat (fd, &sbuf)) ) {
      const char *errorstr =
        "<html><body>An internal server error has occured!\
                              </body></html>";
      /* error accessing file */
      if (fd != -1)
        (void) close (fd);
      response =
        MHD_create_response_from_buffer (strlen (errorstr),
                                         (void *) errorstr,
                                         MHD_RESPMEM_PERSISTENT);
      if (NULL != response) {
          ret =
            MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                response);
          MHD_destroy_response (response);

          return ret;
      }
      else
        return MHD_NO;
  }
  response =
    MHD_create_response_from_fd_at_offset64 (sbuf.st_size, fd, 0);
  MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, content_type);

  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}


int
main (void) {
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                             &answer_to_connection, NULL, MHD_OPTION_END);
  if (NULL == daemon)
    return 1;

  while(1) {};
  MHD_stop_daemon (daemon);
  return 0;
}
