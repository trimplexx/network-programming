#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "jsoncpp.lib")
#pragma comment(lib, "libcurl_imp.lib")
#pragma comment(lib, "crypt32.lib")
#include <curl/curl.h>
#include <json/json.h>
#pragma comment(lib, "ws2_32.lib")

/* funkcja odpowiadająca za wpisywanie wyniku zapytania stringa w formie zakodowanego pliku JSON, zapożyczona z
* /https://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c
*/
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
/* Dane pogodowe do wysłania */
std::string readBuffer;

int readWeather()
{
    /* Deklaracja zmiennych */
    CURL* curl;
    CURLcode res;
    Json::Value js;
    Json::Reader reader;
    int countData = 0;

    curl = curl_easy_init();
    if (curl) {
        /* Sformuowanie zapytania do servera */
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.open-meteo.com/v1/forecast?latitude=50.30&longitude=18.68&current_weather=true&timezone=Europe%2FBerlin");
        /* Wywołanie funkcji 'WriteCallback'*/
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        /* Wskaźnik na string 'readBuffer' */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);

        /* Sprawdzanie czy nie wystąpił błąd podczas zapytania */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

        countData = readBuffer.size();

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return countData;
}

int main(void) {
    setlocale(LC_CTYPE, "Polish");
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    method = TLS_client_method();  /* Tworzenie nowej metody instancji */
    ctx = SSL_CTX_new(method);   /* Tworzenie nowego kontekstu */
    if (ctx == NULL)
    {
        printf("Błąd przy tworzeniu ctx \n");
        return 1;
    }

    /* Otwieranie certyfikatu serwera z pliku */
    if (SSL_CTX_use_certificate_file(ctx, "client.crt", SSL_FILETYPE_PEM) <= 0) {
        perror("Błąd otwierania certyfikatu klienta \n");
        SSL_CTX_free(ctx);
        return 1;
    }
    /* Otwieranie certyfikatu klucza prywatnego serwera z pliku */
    if (SSL_CTX_use_PrivateKey_file(ctx, "client.key", SSL_FILETYPE_PEM) <= 0) {
        perror("Błąd otwierania klucza prytwatnego klienta \n");
        SSL_CTX_free(ctx);
        return 1;
    }
    /* Otwieranie certyfikatu głównego z pliku */
    if (SSL_CTX_load_verify_locations(ctx, "ca.crt", nullptr) <= 0) {
        perror("Nie podano certyfikatu głównego! \n");
        SSL_CTX_free(ctx);
        return 1;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr); // Weryfikuj certyfikat peera
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2); // Wyłącz starą wersję SSL


    WSADATA wsaData;
    /* Zainicjowanie żądanej wersja biblioteki WinSock */
    /* WSAStartup inicjuje użycie Winsock */
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        SSL_CTX_free(ctx);
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    /* Tworzenie gniazda oraz ustawianie protokołu TCP */
    int socketClient;
    socketClient = socket(PF_INET, SOCK_STREAM, 0);
    /* Walidacja funkcji socket */
    /* W razie wystąpienia błędu można zwrócić jego kod */
    if (socketClient == INVALID_SOCKET)
    {
        WSACleanup();
        SSL_CTX_free(ctx);
        return 1;
    }
    /* Struktura w której zawarte są informacje o elementach adresu */
    sockaddr_in server;
    /* Wybór typu jako adresu ipv4 */
    server.sin_family = AF_INET;
    /* Ustawienie numeru portu używanego do przesyłu */
    server.sin_port = htons(7777);
    /* Konwersja adresu sieci internetowej ipv4 na postać binarną */
    int s = inet_pton(AF_INET, "127.0.0.1", &(server.sin_addr));
    /* Walidacja funkcji inet_pton */
    if (s < 1)
    {
        WSACleanup();
        SSL_CTX_free(ctx);
        closesocket(socketClient);
        perror("Błąd konwersji adresu \n");
        return 1;
    }
    /* Rozmiar struktury */
    int server_sizeof = sizeof(server);
    /* Nawiązywanie połączenia z gniazdem */
    int c = connect(socketClient, (struct sockaddr*)&server, server_sizeof);
    /* Walidacja funkcji connect */
    if (c < 0)
    {
        closesocket(socketClient);
        SSL_CTX_free(ctx);
        WSACleanup();
        perror("Błąd połączenia");
        return 1;
    }

    SSL* ssl = SSL_new(ctx);

    //Sprawdzanie poprawności utworzenia struktury.
    if (ssl == 0) {
        printf("Blad podczas szyfrowania!\n");
        closesocket(socketClient);
        WSACleanup();
        SSL_CTX_free(ctx);
    }
    
    // Spróbuj nawiązać połączenie z serverem
    if (SSL_set_fd(ssl, socketClient) != 1 || SSL_connect(ssl) != 1) 
    {
        printf("Blad podczas nawiązywania połączenia\n");
        closesocket(socketClient);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        WSACleanup();
        return 1;
    }

    /* Odczytaj pogode za pomocą CURL */
    int size = readWeather();
    if (size == 0)
    {
        printf("Błąd odczytu danych pogodwych z CURL\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        closesocket(socketClient);
        WSACleanup();
        return 1;
    }
    /* Skopiowanie odczytanego stringa do const char */
    const char* weatherBuff = readBuffer.c_str();

    /* Wysyłanie danych stosując szyfrowanie. */
    int writeData = SSL_write(ssl, weatherBuff, size);

    if (writeData <= 0) {
        printf("Błąd przesyłania danych.\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        closesocket(socketClient);
        WSACleanup();
        return 1;
    }
    else 
      printf("Dane pogodowe zostały pomyślnie przesłane!\n");

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);

    closesocket(socketClient);
    WSACleanup();
    return 0;
}
