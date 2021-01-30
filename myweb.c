#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <microhttpd.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#define PORT 8888

#define BASEDIR "/var/www"
#define INDEX_HTML "index.html"
#define PNGTYPE "image/png"
#define JPGTYPE "image/jpg"
#define ICOTYPE "image/x-icon"
#define HTMLTYPE "text/html"

#define MAP_SIZE 6

struct MHD_Daemon *mht_daemon;
char client_ip[16];

struct cont_element {
  char ext[5];
  char type[20];
} cont_map[MAP_SIZE] = {
			   { ".htm", "text/html" },
			   { "html", "text/html" },
			   { ".ico", "image/x-icon" },
			   { ".png", "image/png" },
			   { ".jpg", "image/jpeg" },
			   { "jpeg", "image/jpeg" }
};

char* get_content_type(char *cont_type,
		     char *file_name) {

  char *ext;
  cont_type = NULL;
  ext = &file_name[strlen(file_name)-4];
  for (int i=0; i < MAP_SIZE; i++)
      if (0 == strcmp(cont_map[i].ext , ext))
	cont_type = cont_map[i].type;
  if (! cont_type)
     cont_type = cont_map[0].type;
  return cont_type;
}

int print_out_key (void *cls,
		   enum MHD_ValueKind kind, 
                   const char *key,
		   const char *value) {
  printf ("%s: %s\n", key, value);
  return MHD_YES;
}


int add_to_log (struct MHD_Connection *connection,
		const char *url,
		const char *method,
                const char *version,
		const char *upload_data,
		size_t *upload_data_size,
		int ret_code) {

  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */

  printf ("%s request from %s for %s using version %s response code %d\n",
	  method, client_ip, url, version, ret_code);
  MHD_get_connection_values (connection, MHD_HEADER_KIND,
			     &print_out_key, NULL);

  return MHD_YES;
}

int construct_filename (char *file_name,
			const char *url,
			const char *basedir) {
  strcpy(file_name, basedir);
  strcpy(file_name, strcat(file_name, url));
  if ( file_name[strlen(file_name)-1] == '/' )
    strcpy(file_name,strcat(file_name, INDEX_HTML));
  return MHD_YES;
}


static int answer_to_connection (void *cls,
		      struct MHD_Connection *connection,
                      const char *url,
		      const char *method,
                      const char *version,
		      const char *upload_data,
                      size_t *upload_data_size,
		      void **con_cls) {

  (void)cls;               /* Unused. Silent compiler warning. */
  (void)con_cls;           /* Unused. Silent compiler warning. */
  
  struct MHD_Response *response;
  int fd;
  int ret;
  struct stat sbuf;
  char file_name[256];
  char *content_type;

 
  if (0 != strcmp (method, "GET"))
    return MHD_NO;

  if (! construct_filename(file_name, url, BASEDIR))
    return MHD_NO;

  content_type = get_content_type(content_type, file_name);
  if (! content_type ) 
    return MHD_NO;
  
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
	  add_to_log(connection,
		     url, method, version,
		     upload_data, upload_data_size,
		     MHD_HTTP_INTERNAL_SERVER_ERROR);
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
  add_to_log(connection,
	     url, method, version,
	     upload_data, upload_data_size,
	     MHD_HTTP_OK);
  return ret;
}

static int on_client_connect (void *cls,
                              const struct sockaddr *addr,
			      socklen_t addrlen) {
  //printf ("sockaddr: %s\n", inet_ntoa(((struct sockaddr_in *)addr)->sin_addr));
  strcpy(client_ip, inet_ntoa(((struct sockaddr_in *)addr)->sin_addr));
  return MHD_YES;
}

void quit_handler(int s) {
  if (NULL != mht_daemon)
    MHD_stop_daemon (mht_daemon);
  exit(0);
}

int main (void) {
  signal(SIGINT, quit_handler);
  signal(SIGTERM, quit_handler);
  mht_daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD,
			     PORT, &on_client_connect, NULL,
                             &answer_to_connection, NULL, MHD_OPTION_END);
  if (NULL == mht_daemon)
    return 1;

  while(1) sleep(1);
  return 0;
}
