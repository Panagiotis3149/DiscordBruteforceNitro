#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <omp.h>
#include <windows.h>


#define WEBHOOK_URL "YOUR WEBHOOK URL"


char* generate_string() {
    static char str[19];
    for (int i = 0; i < 18; i++) {
        if (rand() % 2) {
            str[i] = (rand() % 26) + 'a'; 
        } else {
            str[i] = (rand() % 26) + 'A'; 
        }
        if (rand() % 3 == 0) {
            str[i] = (rand() % 10) + '0'; 
        }
    }
    str[18] = '\0';
    return str;
}

int send_request(CURL* curl, const char* code) {
    CURLcode res;
    char url[256];

    snprintf(url, sizeof(url), "https://discordapp.com/api/v9/entitlements/gift-codes/%s?with_application=false&with_subscription_plan=true", code);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    res = curl_easy_perform(curl);
    if(res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) {
            return 1; 
        }
    }
    return 0;
}


void send_webhook(CURL* curl, const char* code) {
    CURLcode res;
    char payload[256];

    snprintf(payload, sizeof(payload), "{\"content\":\"%s\"}", code);
    curl_easy_setopt(curl, CURLOPT_URL, WEBHOOK_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        fprintf(stderr, "cURL error: %s\n", curl_easy_strerror(res));
    }
}

int main() {
    srand(time(NULL)); 
    int num_cores = omp_get_max_threads();
    printf("CPU CORES: %d\n", num_cores);
    printf("THREADS: %d x\n", num_cores);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    #pragma omp parallel num_threads(num_cores * 4)
    {
        CURL* curl;
        curl = curl_easy_init();

        while (1) {
            char* code = generate_string();
            printf("Attempting to redeem code %s...\n", code);

            if (send_request(curl, code)) {
                printf("Code %s is valid! Sending to webhook...\n", code);
                send_webhook(curl, code);
            } else {
                printf("Code %s is invalid.\n", code);
            }

            
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return 0;
}

