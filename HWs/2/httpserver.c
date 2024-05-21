#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>

#include "libhttp.h"
#include "wq.h"

#define LONG_MAX_NUMBER_OF_DIGITS 20
#define INDEX_FILE "/index.html"
/* Maximum size of the generated html file (1<<20 = 1mb) */
#define MAX_HTML_LENGTH 1 << 30
#define URL_SPACE "%20"

/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
wq_t work_queue;
int num_threads;
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;

char *long_to_string(long int x)
{
  char *str = malloc(sizeof(char) * LONG_MAX_NUMBER_OF_DIGITS);
  sprintf(str, "%ld", x);
  return str;
}

char *file_content_length(char *path)
{
  struct stat st;
  stat(path, &st);
  return long_to_string(st.st_size);
}

char *list_directory_in_html(char *path)
{
  DIR *dir = opendir(path);
  struct dirent *dirent;
  char *html = malloc(sizeof(char) * MAX_HTML_LENGTH);
  strcpy(html, "<div>");
  strcat(html, "<h1>Index of ");
  strcat(html, path);
  strcat(html, "</h1>");

  while ((dirent = readdir(dir)) != NULL)
  {
    strcat(html, "<li><a href=\"");
    strcat(html, dirent->d_name);
    strcat(html, "\">");
    strcat(html, dirent->d_name);
    strcat(html, "</a></li>");
  }

  strcat(html, "</div>");
  return html;
}

void move_string_left(char *str, int step)
{
  for (int i = 0; i < strlen(str); i++)
  {
    str[i] = str[i + step];
  }
}

void replace_url_space(char *path)
{
  char *iter = path;

  while (*iter != '\0')
  {
    if (strncmp(iter, URL_SPACE, strlen(URL_SPACE)) == 0)
    {
      iter[0] = ' ';
      move_string_left(iter + 1, strlen(URL_SPACE) - 1);
    }

    iter++;
  }
}

/*
 * Serves the contents the file stored at `path` to the client socket `fd`.
 * It is the caller's reponsibility to ensure that the file stored at `path` exists.
 * You can change these functions to anything you want.
 *
 * ATTENTION: Be careful to optimize your code. Judge is
 *            sesnsitive to time-out errors.
 */
void serve_file(int fd, char *path)
{
  char *mime_type = http_get_mime_type(path);
  char *content_length_string = file_content_length(path);

  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", mime_type);
  http_send_header(fd, "Content-Length", content_length_string); // Change this too
  http_end_headers(fd);

  /* TODO: PART 1 Bullet 2 */
  size_t content_length = atoi(content_length_string);
  char *content = malloc(sizeof(char) * content_length);
  FILE *f;

  if (strncmp(mime_type, "text", 4) == 0)
  {
    f = fopen(path, "r");
    fread(content, sizeof(char), content_length, f);
    http_send_string(fd, content);
  }
  else
  {
    f = fopen(path, "rb");
    size_t n;

    while ((n = fread(content, sizeof(char), content_length, f)) > 0)
    {
      http_send_data(fd, content, n);
    }
  }

  fclose(f);
  free(content);
  free(content_length_string);
}

void serve_directory(int fd, char *path)
{
  /* TODO: PART 1 Bullet 3,4 */
  char *index_path = malloc(sizeof(char) * (strlen(path) + strlen(INDEX_FILE)));
  strcpy(index_path, path);
  strcat(index_path, INDEX_FILE);

  struct stat s;
  int err = stat(index_path, &s);

  if (!err && S_ISREG(s.st_mode))
  {
    serve_file(fd, index_path);
    free(index_path);
    return;
  }

  free(index_path);
  char *content = list_directory_in_html(path);
  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", http_get_mime_type(".html"));
  http_send_header(fd, "Content-Length", long_to_string(strlen(content)));
  http_end_headers(fd);
  http_send_string(fd, content);
  free(content);
  return;
}

/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 *
 *   Closes the client socket (fd) when finished.
 */
void handle_files_request(int fd)
{

  struct http_request *request = http_request_parse(fd);

  if (request == NULL || request->path[0] != '/')
  {
    http_start_response(fd, 400);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(fd);
    return;
  }

  if (strstr(request->path, "..") != NULL)
  {
    http_start_response(fd, 403);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(fd);
    return;
  }

  /* Remove beginning `./` */
  char *path = malloc(2 + strlen(request->path) + 1);
  path[0] = '.';
  path[1] = '/';
  memcpy(path + 2, request->path, strlen(request->path) + 1);
  replace_url_space(path);

  /*
   * TODO: First is to serve files. If the file given by `path` exists,
   * call serve_file() on it. Else, serve a 404 Not Found error below.
   *
   * TODO: Second is to serve both files and directories. You will need to
   * determine when to call serve_file() or serve_directory() depending
   * on `path`.
   *
   * Feel FREE to delete/modify anything on this function.
   */
  struct stat s;
  int err = stat(path, &s);

  if (!err && S_ISREG(s.st_mode))
  {
    serve_file(fd, path);
    return;
  }

  if (!err && S_ISDIR(s.st_mode))
  {
    serve_directory(fd, path);
    return;
  }

  http_start_response(fd, 404);
  http_send_header(fd, "Content-Type", "text/html");
  http_end_headers(fd);
  close(fd);
  return;
}

typedef struct proxy_info
{
  int src_fd;
  int dst_fd;
  int *is_connection_open;
  pthread_cond_t *cond;
} proxy_info_t;

void *proxy(void *arg)
{
  proxy_info_t info = *(proxy_info_t *)arg;
  char buf[LIBHTTP_REQUEST_MAX_SIZE];
  size_t n;

  while ((*info.is_connection_open) && ((n = read(info.src_fd, buf, LIBHTTP_REQUEST_MAX_SIZE)) > 0))
  {
    http_send_data(info.dst_fd, buf, n);
  }

  *info.is_connection_open = 0;
  pthread_cond_broadcast(info.cond);
  return NULL;
}

void exchange_client_server_data(int client, int server)
{
  int is_connection_open = 1;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  proxy_info_t info = {client, server, &is_connection_open, &cond};
  proxy_info_t reverse_info = {server, client, &is_connection_open, &cond};
  pthread_t proxy_thread;
  pthread_t reverse_proxy_thread;
  pthread_create(&proxy_thread, NULL, proxy, &info);
  pthread_create(&reverse_proxy_thread, NULL, proxy, &reverse_info);

  while (is_connection_open)
  {
    pthread_cond_wait(&cond, mutex);
  }

  pthread_cancel(proxy_thread);
  pthread_cancel(reverse_proxy_thread);
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
}

/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void handle_proxy_request(int fd)
{

  /*
   * The code below does a DNS lookup of server_proxy_hostname and
   * opens a connection to it. Please do not modify.
   */

  struct sockaddr_in target_address;
  memset(&target_address, 0, sizeof(target_address));
  target_address.sin_family = AF_INET;
  target_address.sin_port = htons(server_proxy_port);

  struct hostent *target_dns_entry = gethostbyname2(server_proxy_hostname, AF_INET);

  int target_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (target_fd == -1)
  {
    fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
    close(fd);
    exit(errno);
  }

  if (target_dns_entry == NULL)
  {
    fprintf(stderr, "Cannot find host: %s\n", server_proxy_hostname);
    close(target_fd);
    close(fd);
    exit(ENXIO);
  }

  char *dns_address = target_dns_entry->h_addr_list[0];

  memcpy(&target_address.sin_addr, dns_address, sizeof(target_address.sin_addr));
  int connection_status = connect(target_fd, (struct sockaddr *)&target_address,
                                  sizeof(target_address));

  if (connection_status < 0)
  {
    /* Dummy request parsing, just to be compliant. */
    http_request_parse(fd);

    http_start_response(fd, 502);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    http_send_string(fd, "<center><h1>502 Bad Gateway</h1><hr></center>");
    close(target_fd);
    close(fd);
    return;
  }

  /*
   * TODO: Your solution for task 3 belongs here!
   */
  exchange_client_server_data(fd, target_fd);
  close(target_fd);
}

void *start_thread(void *arg)
{
  void (*request_handler)(int) = arg;

  while (1)
  {
    int fd = wq_pop(&work_queue);
    request_handler(fd);
    close(fd);
  }

  return NULL;
}

void init_thread_pool(int num_threads, void (*request_handler)(int))
{
  /*
   * TODO: Part of your solution for Task 2 goes here!
   */
  pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);

  for (int i = 0; i < num_threads; i++)
  {
    int err = pthread_create(threads + i, NULL, start_thread, request_handler);

    if (err)
    {
      perror("Failed to create thread");
      exit(err);
    }
  }
}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)(int))
{

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1)
  {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
                 sizeof(socket_option)) == -1)
  {
    perror("Failed to set socket options");
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr *)&server_address,
           sizeof(server_address)) == -1)
  {
    perror("Failed to bind on socket");
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1)
  {
    perror("Failed to listen on socket");
    exit(errno);
  }

  printf("Listening on port %d...\n", server_port);

  wq_init(&work_queue);
  init_thread_pool(num_threads, request_handler);

  while (1)
  {
    client_socket_number = accept(*socket_number,
                                  (struct sockaddr *)&client_address,
                                  (socklen_t *)&client_address_length);
    if (client_socket_number < 0)
    {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n",
           inet_ntoa(client_address.sin_addr),
           client_address.sin_port);

    // TODO: Change me?
    wq_push(&work_queue, client_socket_number);

    printf("Accepted connection from %s on port %d\n",
           inet_ntoa(client_address.sin_addr),
           client_address.sin_port);
  }

  shutdown(*socket_number, SHUT_RDWR);
  close(*socket_number);
}

int server_fd;
void signal_callback_handler(int signum)
{
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0)
    perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char *USAGE =
    "Usage: ./httpserver --files www_directory/ --port 8000 [--num-threads 5]\n"
    "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000 [--num-threads 5]\n";

void exit_with_usage()
{
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
  signal(SIGINT, signal_callback_handler);
  signal(SIGPIPE, SIG_IGN);

  /* Default settings */
  server_port = 8000;
  void (*request_handler)(int) = NULL;

  int i;
  for (i = 1; i < argc; i++)
  {
    if (strcmp("--files", argv[i]) == 0)
    {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory)
      {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }

      chdir(server_files_directory);
    }
    else if (strcmp("--proxy", argv[i]) == 0)
    {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target)
      {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL)
      {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      }
      else
      {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    }
    else if (strcmp("--port", argv[i]) == 0)
    {
      char *server_port_string = argv[++i];
      if (!server_port_string)
      {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    }
    else if (strcmp("--num-threads", argv[i]) == 0)
    {
      char *num_threads_str = argv[++i];
      if (!num_threads_str || (num_threads = atoi(num_threads_str)) < 1)
      {
        fprintf(stderr, "Expected positive integer after --num-threads\n");
        exit_with_usage();
      }
    }
    else if (strcmp("--help", argv[i]) == 0)
    {
      exit_with_usage();
    }
    else
    {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL)
  {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
