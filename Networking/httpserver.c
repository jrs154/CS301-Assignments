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
#include <sys/wait.h>
#include <stdbool.h>
#include <libgen.h>

#include "libhttp.h"
#include "wq.h"

/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request.
 * Their values are set up in main() using the command line arguments (already implemented for you).
 */
wq_t work_queue; // Only used by poolserver
int num_threads; // Only used by poolserver
int server_port; // Default value: 8000
char* server_files_directory;


// Read->0, Write->1
ssize_t good_read_write(int flag, int fd, void *buf, size_t count) {
	ssize_t write_byte = 0, read_byte = 0, bytes = 0;
  if(flag==1){
    while (write_byte < count) {
      bytes = write(fd, buf + write_byte, count - write_byte);
      if (bytes < 0) {
        perror("write");
        exit(errno);
      }
      write_byte += bytes;
    }
  }
  else{
    while (read_byte < count) {
      bytes = read(fd, buf + read_byte, count - read_byte);
      if (bytes < 0) {
        perror("read");
        exit(errno);
      }
		  read_byte += bytes;
	  }
  }
	return (flag==1? write_byte:read_byte);
}

/*
 * Serves the contents the file stored at `path` to the client socket `fd`.
 * It is the caller's reponsibility to ensure that the file stored at `path` exists.
 */
void serve_file(int fd, char* path) {

  /* TODO: PART 2 */
  /* PART 2 BEGIN */

  // http_start_response(fd, 200);
  // http_send_header(fd, "Content-Type", http_get_mime_type(path));
  // http_send_header(fd, "Content-Length", "0"); // TODO: change this line too
  // http_end_headers(fd);

  int file;
	struct stat file_stat;
	void *buff_file;
  char buff_file_sz[32];

	http_start_response(fd, 200);
	http_send_header(fd, "Content-Type", http_get_mime_type(path));
	stat(path, &file_stat);
	snprintf(buff_file_sz, 32, "%ld", file_stat.st_size);
	http_send_header(fd, "Content-Length", buff_file_sz);
	http_end_headers(fd);

	buff_file = malloc(file_stat.st_size);
	if (buff_file == NULL) {
		perror("malloc");
		exit(errno);
	}
	file = open(path, O_RDONLY);
	if (file == -1) {
		perror("open");
		free(buff_file);
		exit(errno);
	}
	good_read_write(0,file, buff_file, file_stat.st_size);
	good_read_write(1,fd, buff_file, file_stat.st_size);
	free(buff_file);
	close(file);

  /* PART 2 END */
}

void serve_directory(int fd, char* path) {
  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", http_get_mime_type(".html"));
  http_end_headers(fd);

  /* TODO: PART 3 */
  /* PART 3 BEGIN */

  // TODO: Open the directory (Hint: opendir() may be useful here)

  /**
   * TODO: For each entry in the directory (Hint: look at the usage of readdir() ),
   * send a string containing a properly formatted HTML. (Hint: the http_format_href()
   * function in libhttp.c may be useful here)
   */

  DIR *dir = opendir(path);
	char *buff_entry = malloc(1024);
	char *temp_path_directory = strcpy(malloc(strlen(path)), path);
	char *parent_directory = dirname(temp_path_directory);
  snprintf(buff_entry, 1024, "<a href=\"/%s\">..</a><br/>", parent_directory);
	good_read_write(1,fd, buff_entry, strlen(buff_entry));
	free(temp_path_directory);

  struct dirent *entry;
	for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
		if (entry->d_name[0] != '.') {
			http_format_href(buff_entry, path, entry->d_name);
			good_read_write(1,fd, buff_entry, strlen(buff_entry) + 1);
		}
	}
	free(buff_entry);
	closedir(dir);

  /* PART 3 END */
}

/*
 * Reads an HTTP request from client socket (fd), and writes an HTTP response
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
void handle_files_request(int fd) {

  struct http_request* request = http_request_parse(fd);

  if (request == NULL || request->path[0] != '/') {
    http_start_response(fd, 400);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(fd);

    if (request != NULL)
      free(request->method);
      free(request->path);
      free(request);
    
    return;
  }

  if (strstr(request->path, "..") != NULL) {
    http_start_response(fd, 403);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(fd);

    free(request->method);
    free(request->path);
    free(request);
    return;
  }

  /* Remove beginning `./` */
  char* path = malloc(2 + strlen(request->path) + 1);
  path[0] = '.';
  path[1] = '/';
  memcpy(path + 2, request->path, strlen(request->path) + 1);

  /*
   * TODO: PART 2 is to serve files. If the file given by `path` exists,
   * call serve_file() on it. Else, serve a 404 Not Found error below.
   * The `stat()` syscall will be useful here.
   *
   * TODO: PART 3 is to serve both files and directories. You will need to
   * determine when to call serve_file() or serve_directory() depending
   * on `path`. Make your edits below here in this function.
   */

  /* PART 2 & 3 BEGIN */

  struct stat file_stat;
	int error = stat(path, &file_stat);
	if (error == -1) {
		http_start_response(fd, 404);
		http_send_header(fd, "Content-Type", "text/html");
		http_send_header(fd, "Content-Length", "0");
		http_end_headers(fd);
	}
	else if (S_ISDIR(file_stat.st_mode)) {
		char *index_path = malloc(strlen(path) + 10);
		http_format_index(index_path, path);
		if (stat(index_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode))
			serve_file(fd, index_path);
		else
			serve_directory(fd, path);
		free(index_path);
	}
  else if (S_ISREG(file_stat.st_mode)) {
		serve_file(fd, path);
	}

  /* PART 2 & 3 END */

  free(path);
	close(fd);
  free(request->method);
  free(request->path);
  free(request);

  return;
}


#ifdef POOLSERVER
/*
 * All worker threads will run this function until the server shutsdown.
 * Each thread should block until a new request has been received.
 * When the server accepts a new connection, a thread should be dispatched
 * to send a response to the client.
 */
void* handle_clients(void* void_request_handler) {
  void (*request_handler)(int) = (void (*)(int))void_request_handler;
  /* (Valgrind) Detach so thread frees its memory on completion, since we won't
   * be joining on it. */
  pthread_detach(pthread_self());

  /* TODO: PART 7 */
  /* PART 7 BEGIN */

  /* PART 7 END */

}
/*
 * Creates `num_threads` amount of threads. Initializes the work queue.
 */
void init_thread_pool(int num_threads, void (*request_handler)(int)) {

  /* TODO: PART 7 */
  /* PART 7 BEGIN */

  /* PART 7 END */
}
#endif

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int* socket_number, void (*request_handler)(int)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;

  // Creates a socket for IPv4 and TCP.
  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) ==
      -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  // Setup arguments for bind()
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  /*
   * TODO: PART 1
   *
   * Given the socket created above, call bind() to give it
   * an address and a port. Then, call listen() with the socket.
   * An appropriate size of the backlog is 1024, though you may
   * play around with this value during performance testing.
   */

  /* PART 1 BEGIN */

  if (bind(*socket_number, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
		perror("bind");
		exit(errno);
	}

	if (listen(*socket_number, 1024) == -1) {
		perror("listen");
		exit(errno);
	}

  /* PART 1 END */
  printf("Listening on port %d...\n", server_port);

#ifdef POOLSERVER
  /*
   * The thread pool is initialized *before* the server
   * begins accepting client connections.
   */
  init_thread_pool(num_threads, request_handler);
#endif

  while (1) {
    client_socket_number = accept(*socket_number, (struct sockaddr*)&client_address,
                                  (socklen_t*)&client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n", inet_ntoa(client_address.sin_addr),
           client_address.sin_port);

#ifdef BASICSERVER
    /*
     * This is a single-process, single-threaded HTTP server.
     * When a client connection has been accepted, the main
     * process sends a response to the client. During this
     * time, the server does not listen and accept connections.
     * Only after a response has been sent to the client can
     * the server accept a new connection.
     */
    request_handler(client_socket_number);

#elif THREADSERVER
    /*
     * TODO: PART 4
     *
     * When a client connection has been accepted, a new
     * thread is created. This thread will send a response
     * to the client. The main thread should continue
     * listening and accepting connections. The main
     * thread will NOT be joining with the new thread.
     */

    /* PART 4 BEGIN */

    /* PART 4 END */

#elif POOLSERVER
    /*
     * TODO: PART 5
     *
     * When a client connection has been accepted, add the
     * client's socket number to the work queue. A thread
     * in the thread pool will send a response to the client.
     */

    /* PART 5 BEGIN */

    /* PART 5 END */
#endif
  }

  shutdown(*socket_number, SHUT_RDWR);
  close(*socket_number);
}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0)
    perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char* USAGE =
    "Usage: ./httpserver --files some_directory/ [--port 8000 --num-threads 5]\n";
void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
  signal(SIGINT, signal_callback_handler);
  signal(SIGPIPE, SIG_IGN);

  /* Default settings */
  server_port = 8000;
  void (*request_handler)(int) = NULL;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char* server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--num-threads", argv[i]) == 0) {
      char* num_threads_str = argv[++i];
      if (!num_threads_str || (num_threads = atoi(num_threads_str)) < 1) {
        fprintf(stderr, "Expected positive integer after --num-threads\n");
        exit_with_usage();
      }
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL) {
    fprintf(stderr, "Please specify \"--files [DIRECTORY]\"\n");
    exit_with_usage();
  }

#ifdef POOLSERVER
  if (num_threads < 1) {
    fprintf(stderr, "Please specify \"--num-threads [N]\"\n");
    exit_with_usage();
  }
#endif

  chdir(server_files_directory);
  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
