#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <omp.h>
#include <windows.h>

#define WEBHOOK_URL "YOUR WEBHOOK URL" // change this

uint32_t xorshift_state = 1;
uint32_t xorshift() {
    xorshift_state ^= (xorshift_state << 13);
    xorshift_state ^= (xorshift_state >> 17);
    xorshift_state ^= (xorshift_state << 5);
    return xorshift_state;
}

char* generate_string() {
    static char str[19];
    for (int i = 0; i < 18; i++) {
        int type = xorshift() % 3;
        if (type == 0) {
            str[i] = (xorshift() % 26) + 'a'; 
        } else if (type == 1) {
            str[i] = (xorshift() % 26) + 'A'; 
        } else {
            str[i] = (xorshift() % 10) + '0';
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
    omp_set_num_threads(num_cores * 16); // change this 

    printf("CPU CORES: %d\n", num_cores);
    printf("THREADS: %d x\n", num_cores * 16); // change this

    curl_global_init(CURL_GLOBAL_DEFAULT);

    #pragma omp parallel
    {
        CURL* curl;
        curl = curl_easy_init();

        while (1) {
            char* code = generate_string();

            if (send_request(curl, code)) {
                send_webhook(curl, code);
            }
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return 0;
}
