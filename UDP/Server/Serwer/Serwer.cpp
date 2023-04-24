#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <iostream>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

int main()
{
    setlocale(LC_CTYPE, "Polish");
    WSADATA wsaData;
    /* Zainicjowanie żądanej wersja biblioteki WinSock */
    /* WSAStartup inicjuje użycie Winsock */
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    /* Tworzenie gniazda oraz ustawianie protokołu UDP */
    SOCKET server_socket;
    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    /* Walidacja funkcji socket */
    /* W razie wystąpienia błędu można zwrócić jego kod */
    if (server_socket == INVALID_SOCKET)
    {
        WSACleanup();
        return 1;
    }
    /* Struktura w której zawarte są informacje o elementach adresu */
    struct sockaddr_in server;
    /* Konwersja adresu sieci internetowej ipv4 na postać binarną */
    int s = inet_pton(AF_INET, "127.0.0.1", &(server.sin_addr));
    /* Walidacja funkcji inet_pton */
    if (s < 1)
    {
        printf("Błąd konwersji adresu! \n");
    }
    /* Wybór typu jako adresu ipv4 */
    server.sin_family = AF_INET;
    /* Ustawienie numeru portu używanego do odbioru */
    server.sin_port = htons(4445);
    /* Rozmiar struktury */
    int server_sizeof = sizeof(server);
    /* Tworzenie bufora do przesyłu danych z ustawioną wielkością ramki */
    int const buffer_size = 32000;
    char buffer[buffer_size];
    /* Wiązanie gniazda z adresem za pomocą funkcji bind */
    if (bind(server_socket, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }
    std::cout << "Oczekiwanie na dane..." << std::endl;
    /* Nazwa odbieranego pliku przez serwer */
    FILE* filename;
    filename = fopen("VirtualBox-6.1.4-136177-Win.exe", "wb");
    /* Petla główna odbierająca kolejno ramki danych wraz z ich zapisem */
    while (true)
    {
        /* Walidacja funkcji recvfrom */
        int numbytes = recvfrom(server_socket, buffer, buffer_size, 0, (SOCKADDR*)&server, &server_sizeof);
        if (numbytes == SOCKET_ERROR)
        {
            /* Czyszczenie oraz sprzątanie zasobów w tym zamknięcie pliku */
            printf("Wystąpił błąd przy odbiorze pliku!");
            closesocket(server_socket);
            fclose(filename);
            WSACleanup();
            exit(1);
        }
        /* Zapisywanie przesłanych danych */
        fwrite(buffer, sizeof(char), numbytes, filename);
    }
    /* Czyszczenie oraz sprzątanie zasobów w tym zamknięcie pliku */
    closesocket(server_socket);
    fclose(filename);
    WSACleanup();
    system("pause");
    return 0;
}
