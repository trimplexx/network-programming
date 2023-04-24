#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>
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
    /* Wybór typu jako adres ipv4 */
    server.sin_family = AF_INET;
    /* Ustawienie numeru portu używanego do odbioru */
    server.sin_port = htons(7777);
    /* Ustawienie dowolnego adresu ip */
    server.sin_addr.s_addr = htonl(ADDR_ANY);
    /* Rozmiar struktury */
    int server_sizeof = sizeof(server);
    /* Tworzenie bufora do przesyłu danych z ustawioną wielkością ramki */
    int const buffer_size = 32000;
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
    /* Struktura przechowująca informacje o gnieździe używane przez funkcję WSAPoll */
    struct pollfd pollfdstruc;
    /* Wektor przechowujący przychodzących klientów */
    std::vector<pollfd> socketVector;
    pollfdstruc.fd = server_socket;
    pollfdstruc.events = POLLIN;
    /* Dodanie gniazda serwera do wektora */
    socketVector.push_back(pollfdstruc);
    /* Przechowywany adres klienta */
    char inConnect[INET_ADDRSTRLEN];
    int clientNum = 0;
    /* Tworzenie mapy przechowującej pliki danego klienta */
    std::map<int, FILE*> mapaSkarbow;
    FILE* filename;
    /* Petla główna obsługująca poszczególnych klientów */
    while (true)
    {
        /* Walidacja funkcji WSAPoll */
        int pol = WSAPoll(socketVector.data(), socketVector.size(), -1);
        if (pol < 0)
        {
            perror("Wystąpił błąd funkcji Poll!");
            break;
        }
        if (pol == 0)
        {
            printf("Przekroczono czas oczekiwania! \n");
            break;
        }
        /* Zmienna pomocnicza przypisująca pierwszy element wektora */
        const auto& serv = socketVector.front();
        if (serv.revents & POLLIN)
        {
            /* Akceptacja przychodzącego połączenia wraz z walidacją */
            if ((soc = accept(server_socket, (struct sockaddr*)&clientInStruct, (socklen_t*)&clientInStruct_sizeof)) == INVALID_SOCKET)
            {
                std::cout << "Wystąpił błąd! " << std::endl;
                return 1;
            }
            /* Pobieranie ip klienta poprzez funkcję inet_ntop */
            if (inet_ntop(AF_INET, &(((struct sockaddr_in*)&clientInStruct)->sin_addr), inConnect, clientInStruct_sizeof) == NULL)
            {
                std::cout << "Wystąpił błąd podczas odczytu ip!" << std::endl;
                return 1;
            }
            std::cout << "Połączono z urządzeniem: " << inConnect << std::endl;
            /* Dodanie nowego klienta do wektora */
            pollfdstruc.fd = soc;
            pollfdstruc.events = POLLIN;
            socketVector.push_back(pollfdstruc);
            /* Ustawianie nazwy przesłanego pliku w, którym znajduje się zapisany adres klienta */
            const char* FileNameVariable = "Plik.txt";
            char finalFileName[INET_ADDRSTRLEN + (sizeof(FileNameVariable) / sizeof(char)) + 5];
            strcpy(finalFileName, inConnect);
            strcat(finalFileName, "___");
            /* Konwersja zmiennej odpowiadającej za unikalną nazwę pliku dla każdego z połączonych klientów */
            std::string s = std::to_string(clientNum);
            char const* pchar = s.c_str();
            strcat(finalFileName, pchar);
            clientNum++;
            strcat(finalFileName, FileNameVariable);
            filename = fopen(finalFileName, "wb");
            /* Walidacja pliku */
            if (filename == NULL)
            {
                std::cout << "Podano zły plik ";
                return 1;
            }
            /* Przypisanie do klienta odpowiadającego mu pliku */
            mapaSkarbow.insert(std::pair<int, FILE*>(soc, filename));
        }
        auto i = socketVector.begin() + 1;
        /* Pętla główna odpowiadająca za odczyt oraz zapis do pliku przekazanych danych */
        while (i != socketVector.end())
        {
            if (i->revents & POLLIN)
            {
                numbytes = recv(i->fd, buffer, sizeof(buffer), 0);
                //  std::cout << "Przesyłanie danych!" << std::endl;
                  /* Walidacja funkcji recv */
                if (numbytes == SOCKET_ERROR)
                {
                    std::cout << "Wystąpił błąd!" << std::endl;
                    break;
                }
                /* Zapis do pliku przekazanych danych */
                fwrite(buffer, 1, numbytes, mapaSkarbow.at(i->fd));
                fflush(mapaSkarbow.at(i->fd));
                //   std::cout << "Otrzymano dane!" << std::endl;
                   /* Inkrmentowanej zmiennej odpowiadającej za unikalną nazwę pliku dla każdego z połączonych klientów */
                i++;
            }
            else if (i->revents & (POLLHUP | POLLERR))
            {
                /* Usuwanie zasobów rozłączonego klienta i zamykanie jego pliku */
                std::cout << "Rozłączenie klienta. \n";
                fclose(mapaSkarbow.at(i->fd));
                mapaSkarbow.erase(i->fd);
                closesocket(i->fd);
                i = socketVector.erase(i);
            }
            else i++;
        }
    }
    /* Czyszczenie oraz sprzątanie zasobów */
    closesocket(server_socket);
    WSACleanup();
    return 0;
}