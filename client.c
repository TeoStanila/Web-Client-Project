#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include <string.h>
#include "parson.h"

void register_function(int sockfd)
{
    char **data;
    char *message, *reply;
    data = malloc(2 * sizeof(char *));
    data[0] = malloc(LINELEN * sizeof(char));
    data[1] = malloc(LINELEN * sizeof(char));
    printf("username = ");
    scanf("%s", data[0]);
    printf("password = ");
    scanf("%s", data[1]);
    // Am introdus datele intr-o matrice de stringuri

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "username", data[0]);
    json_object_set_string(root_object, "password", data[1]);
    serialized_string = json_serialize_to_string_pretty(root_value);
    // Am creat obiectul json si stringul trimis catre server

    message = compute_post_request("34.241.4.235", "/api/v1/tema/auth/register", "application/json", serialized_string, NULL, NULL);
    send_to_server(sockfd, message);
    reply = receive_from_server(sockfd);
    if (strstr(reply, "HTTP/1.1 400") != 0)
    {
        printf("Username %s already taken!\n", data[0]);
        // Exita deja contul
    }
    else if (strstr(reply, "HTTP/1.1 201") != 0)
    {
        printf("Registration successful!\n");
        // Succes
    }
    else if (strstr(reply, "429 Too Many Requests") != 0)
    {
        printf("Too many Requests!\n");
    }
    else
    {
        printf("%s\n", reply);
        // In caz ca apare alta eroare
    }
    // Verificam toate cazurile reply-ului

    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
    free(data[0]);
    free(data[1]);
    free(data);
    // Eliberam memoria folosita
}

void login_function(int sockfd, char **cookie)
{

    if (*cookie != NULL)
    {
        printf("You're already logged in!\n");
        return;
    }
    char *message, *reply, **data;
    data = malloc(2 * sizeof(char *));
    data[0] = malloc(LINELEN * sizeof(char));
    data[1] = malloc(LINELEN * sizeof(char));
    printf("username = ");
    scanf("%s", data[0]);
    printf("password = ");
    scanf("%s", data[1]);
    // Am introdus datele intr-o matrice de stringuri

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "username", data[0]);
    json_object_set_string(root_object, "password", data[1]);
    serialized_string = json_serialize_to_string_pretty(root_value);
    // Am creat obiectul json si stringul trimis catre server

    message = compute_post_request("34.241.4.235", "/api/v1/tema/auth/login", "application/json", serialized_string, NULL, NULL);
    send_to_server(sockfd, message);
    reply = receive_from_server(sockfd);
    if (strstr(reply, "HTTP/1.1 200 OK") != 0)
    {
        char *start = (strstr(reply, "Set-Cookie: ") + 12);
        char *final = strstr(start, "; ");
        *cookie = malloc(LINELEN);
        strncpy(*cookie, start, strlen(start) - strlen(final));
        // Extragem cookie-ul

        printf("Login successful!\n");
        return;
    }
    else if (strstr(reply, "HTTP/1.1 400 Bad Request") != 0)
    {
        printf("Credentials are not good!\n");
        *cookie = NULL;
        return;
    }
    else if (strstr(reply, "429 Too Many Requests") != 0)
    {
        printf("Too many Requests!\n");
    }
    else
    {
        printf("%s\n", reply);
        *cookie = NULL;
        return;
    }
}

void enter_library_function(int sockfd, char *cookie, char **token)
{

    if (*token != NULL)
    {
        printf("You already have access!\n");
        return;
    }
    // Verificam daca avem deja acces la biblioteca

    char *message, *reply;
    message = compute_get_request("34.241.4.235", "/api/v1/tema/library/access", cookie, NULL);
    send_to_server(sockfd, message);
    reply = receive_from_server(sockfd);
    if (strstr(reply, "HTTP/1.1 200 OK") != 0)
    {
        char *start = (strstr(reply, "\"token\":\"")) + 9;
        char *final = strstr(start, "\"}");
        *token = malloc(LINELEN);
        strncpy(*token, start, strlen(start) - strlen(final));
        // Construim tokenul

        printf("Acces granted!\n");
        return;
    }
    else if (strstr(reply, "401 Unauthorized") != 0)
    {
        printf("You're not logged in!\n");
        if (*token != NULL)
        {
            free(*token);
        }
        *token = NULL;
        return;
    }
    else if (strstr(reply, "429 Too Many Requests") != 0)
    {
        printf("Too many Requests!\n");
    }
    else
    {
        printf("%s\n", reply);
        free(*token);
        *token = NULL;
        return;
    }
}

void get_books_function(int sockfd, char *token)
{

    if (token == NULL)
    {
        printf("You don't have access!\n");
        return;
    }
    // Verificam daca userul are acces la librarie

    char *message, *reply;
    message = compute_get_request("34.241.4.235", "/api/v1/tema/library/books", NULL, token);
    send_to_server(sockfd, message);
    reply = receive_from_server(sockfd);
    if (strstr(reply, "200 OK") != 0)
    {
        char *booklist = malloc(BUFLEN);
        char *start = strstr(reply, "[{");
        char *final = strstr(reply, "}]");
        if (start == final)
        {
            printf("You don't have any books!\n");
            return;
        }
        strncpy(booklist, start, strlen(start) - strlen(final) + 2);
        printf("%s\n", booklist);
        free(booklist);
    }
    else if (strstr(reply, "429 Too Many Requests") != 0)
    {
        printf("Too many Requests!\n");
    }
    else
    {
        printf("%s\n", reply);
    }
}

void add_book(int sockfd, char *token)
{
    if (token == NULL) {
        printf("Unauthorized access!\n");
        return;
    }
    // Verificam daca avem acces la biblioteca

    char *message, *reply;
    char **data = malloc(5 * sizeof(char *));
    data[0] = malloc(LINELEN);
    data[1] = malloc(LINELEN);
    data[2] = malloc(LINELEN);
    data[3] = malloc(LINELEN);
    data[4] = malloc(LINELEN);
    fgets(data[0], LINELEN, stdin);
    printf("title =  ");
    fgets(data[0], LINELEN, stdin);
    data[0][strlen(data[0]) - 1] = '\0';
    printf("author =  ");
    fgets(data[1], LINELEN, stdin);
    data[1][strlen(data[1]) - 1] = '\0';
    printf("genre = ");
    fgets(data[2], LINELEN, stdin);
    data[2][strlen(data[2]) - 1] = '\0';
    printf("page number = ");
    fgets(data[3], LINELEN, stdin);
    data[3][strlen(data[3]) - 1] = '\0';
    printf("publisher = ");
    fgets(data[4], LINELEN, stdin);
    data[4][strlen(data[4]) - 1] = '\0';
    // Am inregistrat datele despre carte

    for (int i = 0; i < strlen(data[3]); i++)
    {
        if (strchr("1234567890", data[3][i]) == 0)
        {
            printf("Page number is not a number!\n");
            return;
        }
    }
    // Verificam daca numarul de pagini este corect introdus

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "title", data[0]);
    json_object_set_string(root_object, "author", data[1]);
    json_object_set_string(root_object, "genre", data[2]);
    json_object_set_string(root_object, "page_count", data[3]);
    json_object_set_string(root_object, "publisher", data[4]);
    serialized_string = json_serialize_to_string_pretty(root_value);
    // Am creat obiectul json si stringul trimis catere server

    message = compute_post_request("34.241.4.235", "/api/v1/tema/library/books", "application/json", serialized_string, NULL, token);
    send_to_server(sockfd, message);
    reply = receive_from_server(sockfd);
    if (strstr(reply, "403 Forbidden") != 0)
    {
        printf("Unauthorized acces!\n");
        // Inteleg ca nu are cum sa se ajunga aici, dar better safe than sorry
    }
    else if (strstr(reply, "200 OK") != 0)
    {
        printf("Book added to library!\n");
        // Succes
    }
    else if (strstr(reply, "429 Too Many Requests") != 0)
    {
        printf("Too many Requests!\n");
    }
    else
    {
        printf("%s\n", reply);
        // In caz ca apare alt tip de eroare
    }
    free(data);
    free(data[0]);
    free(data[1]);
    free(data[2]);
    free(data[3]);
    free(data[4]);
    return;
}

void delete_book(int sockfd, int book_id, char *token)
{
    if (token == NULL)
    {
        printf("You don't have access!\n");
        return;
    }

    char *message, *reply;
    char real_book_id[5];
    sprintf(real_book_id, "%d", book_id);
    char url[100];
    strcpy(url, "/api/v1/tema/library/books/");
    strcat(url, real_book_id);
    message = compute_delete_request("34.241.4.235", url, NULL, token);
    send_to_server(sockfd, message);
    reply = receive_from_server(sockfd);
    if (strstr(reply, "200 OK") != 0)
    {
        printf("Book removed from library!\n");
    }
    else if (strstr(reply, "404 Not Found") != 0)
    {
        printf("Invalid ID!\n");
    }
    else if (strstr(reply, "429 Too Many Requests") != 0)
    {
        printf("Too many Requests!\n");
    }
    else
    {
        printf("%s\n", reply);
    }
    return;
}

void get_book(int sockfd, int book_id, char *token)
{
    if (token == NULL)
    {
        printf("You don't have access!\n");
        return;
    }

    char *message, *reply;
    char real_book_id[5];

    sprintf(real_book_id, "%d", book_id);
    char url[100];
    strcpy(url, "/api/v1/tema/library/books/");
    strcat(url, real_book_id);
    message = compute_get_request("34.241.4.235", url, NULL, token);
    send_to_server(sockfd, message);
    reply = receive_from_server(sockfd);
    if (strstr(reply, "200 OK"))
    {
        char *bookinfo = malloc(BUFLEN);
        char *start = strstr(reply, "[{");
        char *final = strstr(reply, "}]");
        strncpy(bookinfo, start, strlen(start) - strlen(final) + 2);
        printf("%s\n", bookinfo);
        free(bookinfo);
    }
    else if (strstr(reply, "404 Not Found") != 0)
    {
        printf("Invalid ID!\n");
    }
    return;
}

void logout_function(int sockfd, char **token, char **cookie)
{

    char *message, *reply;
    message = compute_get_request("34.241.4.235", "/api/v1/tema/auth/logout", *cookie, *token);
    send_to_server(sockfd, message);
    reply = receive_from_server(sockfd);
    if (strstr(reply, "200 OK") != 0)
    {
        printf("Logged out!\n");
    }
    else if (strstr(reply, "400 Bad Request") != 0)
    {
        printf("No one's logged in!\n");
    }
    if (*token != NULL)
    {
        free(*token);
        *token = NULL;
    }
    if (*cookie != NULL)
    {
        free(*cookie);
        *cookie = NULL;
    }
    return;
}

int main(int argc, char *argv[])
{
    char input[50];
    int sockfd;
    char *cookie, *token;
    cookie = NULL;
    token = NULL;
    int book_id;

    while (1)
    {
        sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);
        printf("Command: ");
        scanf("%s", input);
        if (strcmp(input, "exit") == 0)
        {
            if (token != NULL)
            {
                free(token);
                token = NULL;
            }
            if (cookie != NULL)
            {
                free(cookie);
                cookie = NULL;
            }
            close(sockfd);
            break;
        }
        else if (strcmp(input, "register") == 0)
        {
            register_function(sockfd);
        }
        else if (strcmp(input, "login") == 0)
        {
            login_function(sockfd, &cookie);
        }
        else if (strcmp(input, "enter_library") == 0)
        {
            enter_library_function(sockfd, cookie, &token);
        }
        else if (strcmp(input, "get_books") == 0)
        {
            get_books_function(sockfd, token);
        }
        else if (strcmp(input, "add_book") == 0)
        {
            add_book(sockfd, token);
        }
        else if (strcmp(input, "delete_book") == 0)
        {
            printf("id = ");
            scanf("%d", &book_id);
            delete_book(sockfd, book_id, token);
        }
        else if (strcmp(input, "get_book") == 0)
        {
            printf("id = ");
            scanf("%d", &book_id);
            get_book(sockfd, book_id, token);
        }
        else if (strcmp(input, "logout") == 0)
        {
            logout_function(sockfd, &token, &cookie);
        }
        else 
        {
            printf("Unknown command, try again!\n");
        }
    }
    return 0;
}