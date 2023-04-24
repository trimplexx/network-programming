#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

int main() {
    setlocale(LC_CTYPE, "Polish");
    WSADATA wsaData;
    /* Zainicjowanie żądanej wersja biblioteki WinSock */
    /* WSAStartup inicjuje użycie Winsock */
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    /* Tworzenie gniazda oraz ustawianie protokołu TCP */
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    /* Walidacja funkcji socket */
    /* W razie wystąpienia błędu można zwrócić jego kod */
    if (server_socket == INVALID_SOCKET)
    {
        WSACleanup();
        return 1;
    }
    /* Struktura w której zawarte są informacje o elementach adresu */
    sockaddr_in server;
    /* Wybór typu jako adresu ipv4 */
    server.sin_family = AF_INET;
    /* Ustawienie numeru portu używanego do odbioru */
    server.sin_port = htons(7777);
    /* Ustawienie dowolnego adresu ip */
    server.sin_addr.s_addr = htonl(ADDR_ANY);
    /* Rozmiar struktury */
    int server_sizeof = sizeof(server);
    /* Tworzenie bufora do przesyłu danych z ustawioną wielkością ramki */
    int const buffer_size = 64000;
    char buffer[buffer_size];
    /* Zmienne pomocnicze */
    int soc;
    int numbytes;
    /* Struktura do przechowywania adresu klienta */
    sockaddr_in clientInStruct;
    /* Rozmiar struktury */
    int clientInStruct_sizeof = sizeof(clientInStruct);
    std::cout << "Oczekwianie na połączenie..." << std::endl;
    /* Wiązanie gniazda z pomocą funkcji bind */
    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        std::cout << "Błąd. Nie można powiązać socketu! " << std::endl;
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }
    /* Nasłuchiwanie połączenia przychodzącego */
    if (listen(server_socket, 10) != 0)
    {
        std::cout << "Wystąpił błąd! " << std::endl;
    }
    /* Petla główna odbierająca kolejno ramki danych wraz z ich zapisem */
    while (1)
    {
        /* Akceptacja przychodzącego połączenia wraz z walidacją */
        if ((soc = accept(server_socket, (struct sockaddr*)&clientInStruct, (socklen_t*)&clientInStruct_sizeof)) == INVALID_SOCKET)
        {
            std::cout << "Wystąpił błąd! " << std::endl;
            return 1;
        }
        /* Przechowywany adres klienta */
        char inConnect[INET_ADDRSTRLEN];
        /* Pobieranie ip klienta poprzez funkcję inet_ntop */
        if (inet_ntop(AF_INET, &(((struct sockaddr_in*)&clientInStruct)->sin_addr), inConnect, clientInStruct_sizeof) == NULL)
        {
            std::cout << "Wystąpił błąd podczas odczytu ip!" << std::endl;
            return 1;
        }
        std::cout << "Połączono z urządzeniem: " << inConnect << std::endl;
        /* Ustawianie nazwy przesłanego pliku w, którym znajduje się zapisany adres klienta */
        const char* FileNameVariable = "VirtualBox.exe";
        char finalFileName[INET_ADDRSTRLEN + (sizeof(FileNameVariable) / sizeof(char)) + 3];
        strcpy(finalFileName, inConnect);
        strcat(finalFileName, "___");
        strcat(finalFileName, FileNameVariable);
        FILE* filename;
        filename = fopen(finalFileName, "wb");
        /* Walidacja pliku */
        if (filename == NULL)
        {
            std::cout << "Podano zły plik ";
            return 1;
        }
        std::cout << "Przesyłanie danych!" << std::endl;
        /* Petla główna odbierająca kolejno ramki danych wraz z ich zapisem */
        while ((numbytes = recv(soc, buffer, sizeof(buffer), 0)) > 0)
        {
            /* Walidacja funkcji recv */
            if (numbytes == SOCKET_ERROR)
            {
                std::cout << "Wystąpił błąd!" << std::endl;
                break;
            }
            fwrite(buffer, 1, numbytes, filename);
        }
        /* Czyszczenie zasobów i czekanie na kolejne połączenie */
        closesocket(soc);
        fclose(filename);
        std::cout << "Otrzymano dane!" << std::endl;
    }
    /* Czyszczenie oraz sprzątanie zasobów w tym zamknięcie pliku */
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
