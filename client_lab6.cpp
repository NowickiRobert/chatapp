#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512            
#define IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT "3504"

struct client_type
{
	SOCKET socket;
	int id;
	char received_message[DEFAULT_BUFLEN];
};

int process_client(client_type &new_client);
int main();

int process_client(client_type &new_client)
{
	while (1)
	{
		memset(new_client.received_message, 0, DEFAULT_BUFLEN);

		if (new_client.socket != 0)
		{
			int iResult = recv(new_client.socket, new_client.received_message, DEFAULT_BUFLEN, 0);

			if (iResult != SOCKET_ERROR)
				cout << new_client.received_message << endl;
			else
			{
				cout << "Blad podczas odbierania danych: " << WSAGetLastError() << endl;
				break;
			}
		}
	}

	if (WSAGetLastError() == WSAECONNRESET)
		cout << "Serwer przestal dzialac" << endl;

	return 0;
}

int main()
{
	WSAData wsa_data;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	string sent_message = "";
	client_type client = { INVALID_SOCKET, -1, "" };
	int iResult = 0;
	string message;

	cout << "Trwa przygotowania polaczenia...\n";

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (iResult != 0) {
		cout << "WSAStartup() error: " << iResult << endl;
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	cout << "Trwa laczenie...\n";

	// Resolve the server address and port
	iResult = getaddrinfo(IP_ADDRESS, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo() nie udalo sie pobrac: " << iResult << endl;
		WSACleanup();
		system("pause");
		return 1;
	}

	// proba ponownych polaczen nim nie nastapi sukces
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		//Tworzenie gnizdka do polaczen z serwerem
		client.socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (client.socket == INVALID_SOCKET) {
			cout << "socket() error: " << WSAGetLastError() << endl;
			WSACleanup();
			system("pause");
			return 1;
		}

		//Trwa laczenie z serwerem.
		iResult = connect(client.socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(client.socket);
			client.socket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (client.socket == INVALID_SOCKET) {
		cout << "Nie mozna polaczyc sie z serwerem!" << endl;
		WSACleanup();
		system("pause");
		return 1;
	}

	cout << "Pomyslnie polaczono z serwerem" << endl;

	//Odbieranie identyfikatora przypisanego przez serwer.
	recv(client.socket, client.received_message, DEFAULT_BUFLEN, 0);
	message = client.received_message;

	if (message != "Serwer jest przepelniony")
	{
		client.id = atoi(client.received_message);

		//odpalenie przetwarzania klienta na watku
		thread my_thread(process_client, client);

		while (1)
		{
			getline(cin, sent_message); // 
			iResult = send(client.socket, sent_message.c_str(), strlen(sent_message.c_str()), 0);

			if (iResult <= 0)
			{
				cout << "send() nie powiodlo sie: " << WSAGetLastError() << endl;
				break;
			}
		}

		//Zamykanie polaczenia jezeli dane nie sa dalej wysylane
		my_thread.detach();
	}
	else
		cout << client.received_message << endl;

	cout << "Zamykanie socketu..." << endl;
	iResult = shutdown(client.socket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown() error: " << WSAGetLastError() << endl;
		closesocket(client.socket);
		WSACleanup();
		system("pause");
		return 1;
	}

	closesocket(client.socket);
	WSACleanup();
	system("pause");
	return 0;
}