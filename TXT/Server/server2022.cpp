#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <filesystem> 

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>
#pragma comment(lib, "ws2_32.lib")

/* Typ wyliczeniowy przechowujący stan operacji */
enum class STATE
{
    /* Oczekiwanie na nagłówek */
    Wait,
    /* Odbieranie danych */
    Upload,
    /* Wysyłanie danych */
    Download
}; 
/* Zmienna pomocnicza */
bool errorFlag = true;
/* Rozmiar nagłówka */
int const sizeHeader = 64;
/* Rozmiar ramki */
int const buffer_size = 4096;
/* Struktura klienta przechowująca jego dane */
struct CLIENT
{
    STATE state;
    SOCKET sock = 0;
    std::string ip;
    FILE* file = nullptr;
    int clientSize = 0;
    char headerBuff[sizeHeader];
    char dataBuff[buffer_size];
};
/* Wektor przechowujący przychodzących klientów */
std::vector<pollfd> socketVector;
/* Tworzenie mapy */
std::map<SOCKET, CLIENT> mapaSkarbow;

/* Funkcja zwalniająca zasoby rozłączanego klienta */
std::vector<pollfd>::iterator disconnect(std::vector<pollfd>::iterator& i)
{    
    if (mapaSkarbow.at(i->fd).file != nullptr)
    {
        fclose(mapaSkarbow.at(i->fd).file);
    }
    mapaSkarbow.erase(i->fd);
    closesocket(i->fd);
    errorFlag = false;
    std::cout << "Rozłączenie klienta. \n";
    return socketVector.erase(i);
}

int main() 
{
    /* Polskie kodowanie znaków */
    setlocale(LC_ALL, "pl_PL");
    /* Zainicjowanie żądanej wersja biblioteki WinSock */
    /* WSAStartup inicjuje użycie Winsock */
    WSADATA wsaData;
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
    SOCKET sock;
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
        return 1;
    }
    /* Nasłuchiwanie połączenia przychodzącego */
    if (listen(server_socket, 10) != 0)
    {
        std::cout << "Wystąpił błąd! " << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;

    }
    /* Struktura przechowująca informacje o gnieździe używane przez funkcję WSAPoll */
    struct pollfd pollfdstruc;
    pollfdstruc.fd = server_socket;
    pollfdstruc.events = POLLIN;
    /* Dodanie gniazda serwera do wektora */
    socketVector.push_back(pollfdstruc);
    /* Przechowywany adres klienta */
    char inConnect[INET_ADDRSTRLEN];
    int clientNum = 0;

    /* Petla główna obsługująca poszczególnych klientów */
    while (true)
    {
        errorFlag = true;
        /* Walidacja funkcji WSAPoll */
        int pol = WSAPoll(socketVector.data(), socketVector.size(), -1);
        if (pol < 0)
        {
            perror("Wystąpił błąd funkcji Poll!");
            closesocket(server_socket);
            WSACleanup();
            return 1;
        }
        /* Zmienna pomocnicza przypisująca pierwszy element wektora */
        const auto& serv = socketVector.front();
        if (serv.revents & POLLIN)
        {
            /* Akceptacja przychodzącego połączenia wraz z walidacją */
            sock = accept(server_socket, (struct sockaddr*)&clientInStruct, (socklen_t*)&clientInStruct_sizeof);
            if (sock != -1)
            {
                /* Pobieranie ip klienta poprzez funkcję inet_ntop */
                if (inet_ntop(AF_INET, &(((struct sockaddr_in*)&clientInStruct)->sin_addr), inConnect, clientInStruct_sizeof) == NULL)
                {
                    std::cout << "Wystąpił błąd podczas odczytu ip!" << std::endl;
                    closesocket(sock);
                }
                std::cout << "Połączono z urządzeniem: " << inConnect << std::endl;
                /* Dodanie nowego klienta do wektora */
                pollfdstruc.fd = sock;
                pollfdstruc.events = POLLIN;
                socketVector.push_back(pollfdstruc);
                mapaSkarbow.insert({ sock, { STATE::Wait, sock, inConnect } });
            }
        }
        auto i = socketVector.begin() + 1;
        /* Pętla główna odpowiadająca za odpowiednie wykonanie przesłanej funkcji przez klienta */
        while (i != socketVector.end())
        {
            if ((mapaSkarbow[i->fd].state == STATE::Wait) && (i->revents & POLLIN))
            {
                /* Obiekt bieżącego klienta */
                auto& client = mapaSkarbow[i->fd];
                int numbytes = recv(i->fd, client.headerBuff + client.clientSize, sizeHeader, 0);
                  /* Walidacja funkcji recv */
                if (numbytes <= 0)
                {
                    std::cout << "Błąd zapisu nagłówka!" << std::endl;
                    i = disconnect(i);
                    break;
                }
                else client.clientSize += numbytes;
                /* Początek bufora */
                const auto pos_start = client.headerBuff;
                /* Koniec bufora */
                const auto pos_end = pos_start + client.clientSize;
                /* Pozycja nowej linii */
                const auto pos_nl = std::find(client.headerBuff, pos_end, '\n');
                if (pos_nl != pos_end)
                {
                    /* Pozycja spacji */
                    const auto pos_sp = std::find(pos_start, pos_nl, ' ');
                    /* Obsługa błedu przesłanego nagłówka */
                    if (pos_sp == pos_nl)
                    {
                        std::cout << "Błąd zapisu nagłówka!" << std::endl;
                        i = disconnect(i);
                        break;
                    }
                    else 
                        *pos_nl = *pos_sp = '\0';
                    /* Sekcja odpowiedzialna za pobieranie danych od klienta */
                    if (strcmp(pos_start, "UPLOAD") == 0)
                    {  
                        /* Pobieranie nazwy podanego pliku */
                        const std::string readFilename(pos_sp + 1);
                        /* Obsługa błędu */
                        if (readFilename == "")
                        {
                            std::cout << "Błędny nagłówek" << std::endl;
                            i = disconnect(i);
                            break;
                        }
                        const std::string minusLastChar = readFilename.substr(0, readFilename.size() - 1 );
                        const std::string name = mapaSkarbow[i->fd].ip + "_" + std::to_string(i->fd) + "_" + minusLastChar;
                        const char* filename = name.c_str();
                        /* Otwarcie do zapisu pliku o zadanej nazwie */
                        client.file = fopen(filename, "wb");
                        /* W przypadku błędu zakończ połączenie */
                        if (client.file == nullptr) {
                            std::cout << "Błąd otwierania pliku!" << std::endl;
                            i = disconnect(i);
                            break;
                        }
                        /* Czyszczenie buforu */
                        memset(client.headerBuff, '\0', client.clientSize);
                        /* Zmiana stanu */
                        client.state = STATE::Upload;
                    }
                    /* Sekcja odpowiedzialna za wysyłanie danych do klienta */
                    else if (strcmp(pos_start, "DOWNLOAD") == 0)
                    {
                        /* Pobranie nazwy pliku */
                        const std::string name(pos_sp + 1);
                        /* Obsługa błędu */
                        if (name == "")
                        {
                            std::cout << "Błędny nagłówek" << std::endl;
                            i = disconnect(i);
                            break;
                        }
                        const char* filename = name.c_str();
                        /* Otwarcie do odczytu danego pliku */
                        client.file = fopen(filename, "rb");
                        /* Obsługa błedu oraz kończenie połączenia */
                        if (client.file == nullptr) {
                            std::cout << "Nie znaleziono pliku" << std::endl;
                            i = disconnect(i);
                            break;
                        }
                        /* Czyszczenie buforu */
                        memset(client.headerBuff, '\0', client.clientSize);
                        /* Zmiana stanu na wysyłanie pliku */
                        client.state = STATE::Download;
                        /* Sprawdzenie gotowości do zapisu danych */
                        i->events |= POLLOUT;
                    }
                    /* Kończenie połączenia w końcowym przypadku */
                    else
                    {
                        std::cout << "Błąd otwierania pliku!" << std::endl;
                        i = disconnect(i);
                        break;
                    };
                }
                /* Ustawianie flagi pomocniczej */
                if (errorFlag == true)
                {
                    i++;
                }
                
            }
            /* Obsługa funkcji odbierania danych od klienta */
            else if ((mapaSkarbow[i->fd].state == STATE::Upload) && (i->revents & POLLIN))
            {
                auto& client = mapaSkarbow[i->fd];
                int numbytes = recv(i->fd, client.dataBuff, sizeof(client.dataBuff), 0);
                /* Walidacja funkcji recv */
                if (numbytes <= 0) {
                    std::cout << "Błąd w otrzymywaniu danych z pliku" << std::endl;
                    i = disconnect(i);
                    break;
                }
                else
                {
                    /* Obsługa błędu oraz rozłączenie klienta */
                    if (fwrite(client.dataBuff, 1, numbytes, client.file) < 0) {
                        std::cout << "Błąd zapisu danych" << std::endl;
                        i = disconnect(i);
                    }
                    /* Zapis danych od klienta */
                    else 
                    {
                        std::cout << "Zapisano dane" << std::endl;
                        fflush(client.file);
                    }
                }
                /* Czyszczenie buforu */
                memset(client.dataBuff, '\0', sizeof(client.dataBuff));
                /* Ustawianie flagi pomocniczej */
                if (errorFlag == true)
                {
                    i++;
                }
            }
            /* Obsługa funkcji wysyłania danych do klienta */
            else if (mapaSkarbow[i->fd].state == STATE::Download && i->revents & POLLOUT)
            {
                auto& client = mapaSkarbow[i->fd];
                int numbytes = fread(client.dataBuff, 1, sizeof(client.dataBuff), client.file);
                if (numbytes < 0)
                {
                    std::cout << "Błąd odczytu danych";
                    i = disconnect(i);
                }
                else if (numbytes == 0)
                {
                    std::cout << "Dane zostały wczytane" << std::endl;
                    i = disconnect(i);
                }
                else
                {
                    int sent = send(i->fd, client.dataBuff, numbytes, 0);
                    if (sent <= 0)  i = disconnect(i);
                }
                /* Ustawianie flagi pomocniczej */
                if (errorFlag == true)
                {
                    i++;
                }
            }
            else if (i->revents & (POLLHUP | POLLERR))
            {
                /* Usuwanie zasobów rozłączonego klienta i zamykanie jego pliku */
                i = disconnect(i);
            }
            else i++;
        }
    }
    /* Czyszczenie oraz sprzątanie zasobów */
    closesocket(server_socket);
    WSACleanup();
    socketVector.clear();
    mapaSkarbow.clear();
    return 0;
}
