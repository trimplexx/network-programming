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
    int socketClient;
    socketClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    /* Walidacja funkcji socket */
    /* W razie wystąpienia błędu można zwrócić jego kod */
    if (socketClient == INVALID_SOCKET)
    {
        WSACleanup();
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
        printf("Błąd konwersji adresu \n");
        return 1;
    }
    /* Rozmiar struktury */
    int server_sizeof = sizeof(server);
    /* Nawiązywanie połączenia z gniazdem */
    int c = connect(socketClient, (struct sockaddr*)&server, server_sizeof);
    /* Walidacja funkcji connect */
    if (c < 0)
    {
        perror("Błąd połączenia");
        closesocket(socketClient);
        WSACleanup();
        return 1;
    }
    /* Tworzenie bufora do przesyłu danych z ustawioną wielkością ramki */
    int const buffer_size = 64000;
    char buffer[buffer_size];
    /* Nazwa wysyłanego pliku przez klienta */
    FILE* filename;
    filename = fopen("VirtualBox-6.1.4-136177-Win.exe", "rb");
    /* Walidacja czy podany plik istnieje */
    if (filename == NULL)
    {
        std::cout << "Podano zły plik!";
        exit(1);
    }
    /* Przejście do główwnej pętli odpowiadającej za przesyłanie pliku */
    if (filename != NULL)
    {
        /* Początek odliczania czasu */
        clock_t start, finish;
        start = clock();
        int temp;
        /* Petla przesyłajaca kolejno ramki danych pliku */
        while ((temp = fread(&buffer, 1, buffer_size, filename)) > 0)
        {
            int numbytes = send(socketClient, buffer, temp, 0);
            if (numbytes == SOCKET_ERROR)
            {
                printf("Nie udało sie przesłać pliku!");
                closesocket(socketClient);
                fclose(filename);
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
        perror("Nie udało się otworzyć pliku");
        fclose(filename);
        closesocket(socketClient);
        WSACleanup();
        return 1;
    }
    /* Czyszczenie oraz sprzątanie zasobów w tym zamknięcie pliku */
    closesocket(socketClient);
    WSACleanup();
    return 0;
}
