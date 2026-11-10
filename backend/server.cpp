
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <cmath>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Bank.h"

using namespace std;

// DECODE URL DATA
string urlDecode(string value)
{
    string result;

    for (int i = 0; i < value.length(); i++)
    {
        if (value[i] == '+')
        {
            result += ' ';
        }
        else if (value[i] == '%' && i + 2 < value.length())
        {
            string hex = value.substr(i + 1, 2);

            try
            {
                int number = stoi(hex, nullptr, 16);
                result += (char)number;
                i += 2;
            }
            catch (...)
            {
                result += value[i];
            }
        }
        else
        {
            result += value[i];
        }
    }

    return result;
}

// GET FORM VALUE
string getValue(string body, string key)
{
    string search = key + "=";
    size_t position = body.find(search);

    if (position == string::npos)
        return "";

    position += search.length();

    size_t end = body.find('&', position);

    if (end == string::npos)
        end = body.length();

    return urlDecode(body.substr(position, end - position));
}

// ESCAPE TEXT FOR JSON
string jsonEscape(string value)
{
    string result;

    for (int i = 0; i < value.length(); i++)
    {
        char ch = value[i];

        if (ch == '"')
            result += "\\\"";
        else if (ch == '\\')
            result += "\\\\";
        else if (ch == '\n')
            result += "\\n";
        else if (ch == '\r')
            result += "\\r";
        else if (ch == '\t')
            result += "\\t";
        else
            result += ch;
    }

    return result;
}

// VALIDATE AMOUNT
bool getAmount(string value, double& amount)
{
    try
    {
        size_t used = 0;
        amount = stod(value, &used);

        return used == value.length() &&
               isfinite(amount) &&
               amount > 0;
    }
    catch (...)
    {
        return false;
    }
}

// GET CONTENT TYPE
string getContentType(string path)
{
    if (path.find(".css") != string::npos)
        return "text/css";

    if (path.find(".js") != string::npos)
        return "application/javascript";

    return "text/html";
}

// SEND HTTP RESPONSE
void sendResponse(SOCKET client, string status,
                  string contentType, string body)
{
    string response =
        "HTTP/1.1 " + status + "\r\n" +
        "Content-Type: " + contentType + "\r\n" +
        "Content-Length: " + to_string(body.size()) + "\r\n" +
        "Connection: close\r\n\r\n" + body;

    send(client, response.c_str(), response.length(), 0);
}

int main()
{
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    sockaddr_in serverAddress;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cout << "Winsock initialization failed!" << endl;
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET)
    {
        cout << "Socket creation failed!" << endl;
        WSACleanup();
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddress,
             sizeof(serverAddress)) == SOCKET_ERROR)
    {
        cout << "Port binding failed!" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    listen(serverSocket, 5);

    Bank bank;
    bank.loadAccounts();

    cout << "SJNA Bank server started!" << endl;
    cout << "Open http://127.0.0.1:8080" << endl;

    while (true)
    {
        clientSocket = accept(serverSocket, NULL, NULL);

        if (clientSocket == INVALID_SOCKET)
            continue;

        char buffer[4096] = {0};

        int received = recv(clientSocket, buffer,
                            sizeof(buffer) - 1, 0);

        if (received <= 0)
        {
            closesocket(clientSocket);
            continue;
        }

        string request(buffer, received);

        size_t headerEnd = request.find("\r\n\r\n");

        if (headerEnd == string::npos)
        {
            sendResponse(clientSocket, "400 Bad Request",
                         "text/plain", "Invalid request");
            closesocket(clientSocket);
            continue;
        }

        string headers = request.substr(0, headerEnd);
        string body = request.substr(headerEnd + 4);

        size_t lengthPosition = headers.find("Content-Length:");

        if (lengthPosition != string::npos)
        {
            lengthPosition += 15;

            while (lengthPosition < headers.length() &&
                   headers[lengthPosition] == ' ')
                lengthPosition++;

            size_t lengthEnd = headers.find("\r\n", lengthPosition);

            int contentLength = stoi(
                headers.substr(lengthPosition,
                               lengthEnd - lengthPosition));

            while (body.length() < contentLength)
            {
                char extra[4096];

                int count = recv(clientSocket, extra,
                                 sizeof(extra), 0);

                if (count <= 0)
                    break;

                body.append(extra, count);
            }
        }

        size_t firstSpace = request.find(' ');
        size_t secondSpace = request.find(' ', firstSpace + 1);

        if (firstSpace == string::npos ||
            secondSpace == string::npos)
        {
            closesocket(clientSocket);
            continue;
        }

        string method = request.substr(0, firstSpace);

        string path = request.substr(
            firstSpace + 1,
            secondSpace - firstSpace - 1);

        size_t queryPosition = path.find('?');

        if (queryPosition != string::npos)
            path = path.substr(0, queryPosition);

        // ACCOUNT REGISTRATION
        if (path == "/api/register" && method == "POST")
        {
            string number = getValue(body, "accountNumber");
            string name = getValue(body, "accountHolder");
            string pin = getValue(body, "pin");
            string accountType = getValue(body, "accountType");

            bool valid = !number.empty() &&
                         !name.empty() &&
                         pin.length() == 4 &&
                         (accountType == "Savings" ||
                          accountType == "Current");

            for (int i = 0; i < pin.length(); i++)
            {
                if (pin[i] < '0' || pin[i] > '9')
                    valid = false;
            }

            if (!valid)
            {
                sendResponse(clientSocket, "400 Bad Request",
                    "application/json",
                    "{\"success\":false,\"message\":\"Please enter valid account details.\"}");
            }
            else if (bank.registerAccount(number, name, pin, accountType))
            {
                sendResponse(clientSocket, "200 OK",
                    "application/json",
                    "{\"success\":true,\"message\":\"Account created successfully!\"}");
            }
            else
            {
                sendResponse(clientSocket, "409 Conflict",
                    "application/json",
                    "{\"success\":false,\"message\":\"Account number already exists.\"}");
            }
        }

        // ACCOUNT LOGIN
        else if (path == "/api/login" && method == "POST")
        {
            string number = getValue(body, "accountNumber");
            string pin = getValue(body, "pin");

            if (bank.login(number, pin))
            {
                sendResponse(clientSocket, "200 OK",
                    "application/json",
                    "{\"success\":true,\"message\":\"Login successful!\"}");
            }
            else
            {
                sendResponse(clientSocket, "401 Unauthorized",
                    "application/json",
                    "{\"success\":false,\"message\":\"Invalid account number or PIN.\"}");
            }
        }

        // GET ACCOUNT DETAILS
        else if (path == "/api/account" && method == "POST")
        {
            string number = getValue(body, "accountNumber");
            string name;
            string accountType;
            double balance;

            if (bank.getAccountDetails(number, name, balance) &&
                bank.getAccountType(number, accountType))
            {
                string response =
                    "{\"success\":true,\"accountHolder\":\"" +
                    jsonEscape(name) +
                    "\",\"accountType\":\"" +
                    jsonEscape(accountType) + "\"}";

                sendResponse(clientSocket, "200 OK",
                    "application/json", response);
            }
            else
            {
                sendResponse(clientSocket, "404 Not Found",
                    "application/json",
                    "{\"success\":false,\"message\":\"Account not found.\"}");
            }
        }

        // CHECK ACCOUNT BALANCE
        else if (path == "/api/balance" && method == "POST")
        {
            string number = getValue(body, "accountNumber");

            double balance = bank.checkBalance(number);

            if (balance < 0)
            {
                sendResponse(clientSocket, "404 Not Found",
                    "application/json",
                    "{\"success\":false,\"message\":\"Account not found.\"}");
            }
            else
            {
                ostringstream result;
                result << fixed << setprecision(2) << balance;

                string response =
                    "{\"success\":true,\"balance\":" +
                    result.str() + "}";

                sendResponse(clientSocket, "200 OK",
                    "application/json", response);
            }
        }

        // DEPOSIT MONEY
        else if (path == "/api/deposit" && method == "POST")
        {
            string number = getValue(body, "accountNumber");
            string amountText = getValue(body, "amount");
            double amount;

            if (!getAmount(amountText, amount))
            {
                sendResponse(clientSocket, "400 Bad Request",
                    "application/json",
                    "{\"success\":false,\"message\":\"Enter a valid amount.\"}");
            }
            else if (bank.depositMoney(number, amount))
            {
                sendResponse(clientSocket, "200 OK",
                    "application/json",
                    "{\"success\":true,\"message\":\"Deposit successful!\"}");
            }
            else
            {
                sendResponse(clientSocket, "400 Bad Request",
                    "application/json",
                    "{\"success\":false,\"message\":\"Deposit failed. Check the account details.\"}");
            }
        }

        // WITHDRAW MONEY
        else if (path == "/api/withdraw" && method == "POST")
        {
            string number = getValue(body, "accountNumber");
            string amountText = getValue(body, "amount");
            double amount;

            if (!getAmount(amountText, amount))
            {
                sendResponse(clientSocket, "400 Bad Request",
                    "application/json",
                    "{\"success\":false,\"message\":\"Enter a valid amount.\"}");
            }
            else if (bank.withdrawMoney(number, amount))
            {
                sendResponse(clientSocket, "200 OK",
                    "application/json",
                    "{\"success\":true,\"message\":\"Withdrawal successful!\"}");
            }
            else
            {
                sendResponse(clientSocket, "400 Bad Request",
                    "application/json",
                    "{\"success\":false,\"message\":\"Withdrawal failed. Check your balance.\"}");
            }
        }

        // TRANSFER MONEY
        else if (path == "/api/transfer" && method == "POST")
        {
            string sender = getValue(body, "sender");
            string receiver = getValue(body, "receiver");
            string amountText = getValue(body, "amount");
            double amount;

            if (!getAmount(amountText, amount))
            {
                sendResponse(clientSocket, "400 Bad Request",
                    "application/json",
                    "{\"success\":false,\"message\":\"Enter a valid amount.\"}");
            }
            else if (sender == receiver)
            {
                sendResponse(clientSocket, "400 Bad Request",
                    "application/json",
                    "{\"success\":false,\"message\":\"You cannot transfer money to your own account.\"}");
            }
            else if (bank.transferMoney(sender, receiver, amount))
            {
                sendResponse(clientSocket, "200 OK",
                    "application/json",
                    "{\"success\":true,\"message\":\"Transfer successful!\"}");
            }
            else
            {
                sendResponse(clientSocket, "400 Bad Request",
                    "application/json",
                    "{\"success\":false,\"message\":\"Transfer failed. Check the receiver account and available balance.\"}");
            }
        }

        // TRANSACTION HISTORY
        else if (path == "/api/transactions" && method == "POST")
        {
            string number = getValue(body, "accountNumber");

            string name;
            double balance;

            if (!bank.getAccountDetails(number, name, balance))
            {
                sendResponse(clientSocket, "404 Not Found",
                    "application/json",
                    "{\"success\":false,\"message\":\"Account not found.\"}");
            }
            else
            {
                string transactions = bank.getTransactions(number);

                string response =
                    "{\"success\":true,\"transactions\":\"" +
                    jsonEscape(transactions) + "\"}";

                sendResponse(clientSocket, "200 OK",
                    "application/json", response);
            }
        }

        // WEBSITE PAGES
        else
        {
            if (path == "/")
                path = "/index.html";

            if (path.find("..") != string::npos)
                path = "/404.html";

            string filePath = "frontend" + path;

            ifstream file(filePath.c_str(), ios::binary);

            if (file)
            {
                stringstream content;
                content << file.rdbuf();

                string page = content.str();
                file.close();

                sendResponse(clientSocket, "200 OK",
                             getContentType(path), page);
            }
            else
            {
                sendResponse(clientSocket, "404 Not Found",
                    "text/html",
                    "<html><body><h1>404 - Page Not Found</h1></body></html>");
            }
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
