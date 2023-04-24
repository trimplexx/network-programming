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
    /* Ustawienie polskiego kodowania */
    setlocale(LC_ALL, "pl_PL");
    WSADATA wsaData;
    /* Zainicjowanie żądanej wersja biblioteki WinSock */
    /* WSAStartup inicjuje użycie Winsock */
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    /* Tworzenie gniazda oraz ustawianie protokołu TCP */
    SOCKET socketClient;
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
    int const buffer_size = 4096;
    char buffer[buffer_size];

    /* Nazwa wysyłanego pliku przez klienta */
    FILE* file;
    /* Zmienne pomocnicze do obsługi nagłówka */
    std::string header;
    std::string filename;
    std::cout << "Wprowadz czynnosc jaka chcesz wykonac ('UPLOAD' bądź 'DOWNLOAD'): ";
    std::cin >> header;
    /* Jeżeli wprowadzimy zły header wyrzuci błąd oraz rozłaczy klienta */
    if (header != "UPLOAD" && header != "DOWNLOAD")
    {
        std::cout << "Podałeś błędnny nagłowek";
        closesocket(socketClient);
        WSACleanup();
        return 1;
    }
    /* Gdy header będzie równy uploadowi danych, to nastąpi wprowadznie pliku, który chcemy wysłać w nagłówek */
    else if (header == "UPLOAD")
    {
        std::cout << "Podaj plik, który chcesz wysłać: ";
        std::cin >> filename;
        /* Przerobienie operacji oraz nazwy pliku na odpowiedni format */
        const std::string headerAndName = header + " " + filename + " \n";
        const char* headerAndNameChar = headerAndName.c_str();
        /* Iteracja po charze, aby sprawdzić ile znaków znajduje się w nagłówku */
        int Size = 0;
        while (headerAndNameChar[Size] != '\0') Size++;
        /* Wysłanie nagłówka do servera */
        send(socketClient, headerAndNameChar, Size, 0);
        /* Otworzenie pliku, który chcemy przesłać */
        const char* finalFileName = filename.c_str();
        file = fopen(finalFileName, "rb");
        /* Jeżeli plik zostanie źle otworzony zamykamy połączenie klienta */
        if (file == nullptr)
        {
            std::cout << "Błąd w tworzeniu pliku";
            closesocket(socketClient);
            WSACleanup();
            return 1;
        }
        else
        {
            /* Pomiar czasu przesyłanych danych */
            clock_t start, finish;
            start = clock();
            int temp;
            /* Petla przesyłajaca kolejno ramki danych pliku */
            while ((temp = fread(&buffer, 1, buffer_size, file)) > 0)
            {
                int numbytes = send(socketClient, buffer, temp, 0);
                if (numbytes == SOCKET_ERROR)
                {
                    printf("Nie udało sie przesłać pliku!");
                    closesocket(socketClient);
                    fclose(file);
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
            fclose(file);
            WSACleanup();
        }
    }
    /* Obsługa pobierania pliku z servera */
    else if (header == "DOWNLOAD")
    {
        std::cout << "Podaj plik, który chcesz pobrać: ";
        std::cin >> filename;
        /* Header wysyłany do servera */
        const std::string headerAndName = header + " " + filename + " \n";
        const char* headerAndNameChar = headerAndName.c_str();
        /* Iteracja po charze, aby sprawdzić ile znaków znajduje się w nagłówku */
        int Size = 0;
        while (headerAndNameChar[Size] != '\0') Size++;
        /* Wysłanie nagłówka do servera */
        send(socketClient, headerAndNameChar, Size, 0);
        /* Przekopiowywanie nazwy wpisanego pliku do char */
        const char* finalFileName = filename.c_str();
        file = fopen(finalFileName, "wb");
        /* Walidacja otworzenia pliku */
        if (file == nullptr)
        {
            std::cout << "Błąd w tworzeniu pliku";
            closesocket(socketClient);
            WSACleanup();
            return 1;
        }
        else
        {
            int count = 0;
            int numbytes = 0;
            /* Pętla odpowiadająca za pobieranie otrzymywanych danych */
            while ((numbytes = recv(socketClient, buffer, sizeof(buffer), 0)) > 0)
            {
                /* Zapisz dane do pliku */
                fwrite(buffer, 1, numbytes, file);
                /* Dodaj do licznika długość danych */
                count += numbytes;
            }
            if (numbytes < 0)
            {
                std::cout << "Odczytu danych";
                closesocket(socketClient);
                fclose(file);
                WSACleanup();
                return 1;
            }
            /* Zamknięcie pliku */
            fclose(file);
        }

    }
    /* Czyszczenie oraz sprzątanie zasobów w tym zamknięcie pliku */
    closesocket(socketClient);
    WSACleanup();
    return 0;
}
