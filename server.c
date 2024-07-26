#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>

// Define constants
#define SERVER_CERT "server.crt"
#define SERVER_KEY "server.key"
#define SERVER_PORT 443

// Function to handle client connections
void handle(SSL *ssl) {
    // Define HTTP response
    char res[] = "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "\r\n"
                 "<html><body><h1>HTTPS SERVER ON CRACK</h1></body></html>\r\n";
    
    // Send response over SSL connection
    SSL_write(ssl, res, strlen(res));
    
    // Clean up SSL connection
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

int main() {
    SSL_CTX *ctx;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    SSL *ssl;

    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    // Create a new SSL context
    ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Load the server certificate
    if (SSL_CTX_use_certificate_file(ctx, SERVER_CERT, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Load the private key
    if (SSL_CTX_use_PrivateKey_file(ctx, SERVER_KEY, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind the socket to the server address
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_fd, 1) < 0) {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    // Main server loop
    while(1) {
        // Accept a client connection
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("Unable to accept");
            continue;
        }

        // Create a new SSL structure for this connection
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);

        // Perform SSL handshake
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            // Handle the client request
            handle(ssl);
        }

        // Close the client socket
        close(client_fd);
    }

    // Clean up (Note: This part is never reached in this infinite loop)
    SSL_CTX_free(ctx);
    close(server_fd);
    return 0;
}