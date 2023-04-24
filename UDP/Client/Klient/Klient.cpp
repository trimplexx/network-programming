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
    SOCKET socketClient;
    socketClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    /* Walidacja funkcji socket */
    /* W razie wystąpienia błędu można zwrócić jego kod */
    if (socketClient == INVALID_SOCKET)
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
    /* Ustawienie numeru portu używanego do przesyłu */
    server.sin_port = htons(4445);
    /* Rozmiar struktury */
    int server_sizeof = sizeof(server);
    /* Tworzenie bufora do przesyłu danych z ustawioną wielkością ramki */
    int const buffer_size = 32000;
    char buffer[buffer_size];
    /* Nazwa wysyłanego pliku przez klienta */
    FILE* filename;
    filename = fopen("VirtualBox-6.1.4-136177-Win.exe", "rb");
    /* Walidacja czy podany plik istnieje */
    if (filename == NULL)
    {
        std::cout << "Podano złą nazwę pliku!";
        return 1;
    }
    /* Przejście do główwnej pętli odpowiadającej za przesyłanie pliku */
    if (filename != NULL)
    {
        /* Początek odliczania czasu */
        clock_t start, finish;
        start = clock();
        /* Petla przesyłajaca kolejno ramki danych pliku */
        while (fread(&buffer, 1, buffer_size, filename) > 0)
        {
            /* Walidacja funkcji sendto */
            if (int numbytes = sendto(socketClient, buffer, buffer_size, 0, (SOCKADDR*)&server, server_sizeof) == SOCKET_ERROR)
            {
                printf("Nie udalo sie przesłac pliku!");
                closesocket(socketClient);
                WSACleanup();
                exit(1);
            }
        }
        /* Koniec odliczania czasu */
        finish = clock();
        std::cout << "Dane zostały wysłane!" << std::endl;
        /* Obliczanie końcowego czasu całego transferu danych */
        double duration = (double)(finish - start) / CLOCKS_PER_SEC;
        std::cout << "Czas transferu: " << duration << " s" << std::endl;
        /* Czyszczenie oraz sprzątanie zasobów w tym zamknięcie pliku */
        closesocket(socketClient);
        fclose(filename);
        WSACleanup();
    }
    else
    {
        /* Czyszczenie oraz sprzątanie zasobów w tym zamknięcie pliku */
        perror("Wystąpił błąd!");
        fclose(filename);
        closesocket(socketClient);
        WSACleanup();
        return 1;
    }
    return 0;
}
