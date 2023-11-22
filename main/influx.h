#pragma once
#include <string>
#include <vector>
#include <ctime>  // for time_t
#include <esp_log.h>
#include <esp_http_client.h>

namespace Influx
{
    class Client
    {
        constexpr const static char* TAG = "Influx::Client";
        const std::string url;
        const std::string token;
        const std::string organisation;
        const std::string bucket;

        esp_http_client_config_t config = {};
        esp_http_client_handle_t client;

        static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
        {
            return ESP_OK;
        }

    public:

        Client(const std::string url, const std::string token, const std::string organisation, const std::string bucket)
            : url(url)
            , token(token)
            , organisation(organisation)
            , bucket(bucket)
            , config{
                .host = url.c_str(),
                .path = "/api/v2/write",
                .query = "",
                .disable_auto_redirect = true,
                .event_handler = _http_event_handler
            }
        {
            client = esp_http_client_init(&config);
            esp_http_client_set_header(client, "Accept", "application/json");
            esp_http_client_set_header(client, "Content-Type", "text/plain; charset=utf-8");

            std::string authHeader = "Token " + token;
            esp_http_client_set_header(client, "Authorization", authHeader.c_str());
        }

        ~Client()
        {
            esp_http_client_cleanup(client);
        }

        int SendLine(std::string line)
        {
            const char *post_data = line.c_str();
            std::string writeUrl = url + "/api/v2/write" + "?org=" + organisation + "&bucket=" + bucket + "&precision=ns";
            esp_http_client_set_url(client, writeUrl.c_str());
            esp_http_client_set_method(client, HTTP_METHOD_POST);
            esp_http_client_set_post_field(client, post_data, strlen(post_data));

            ESP_LOGD(TAG, "Query = '%s'", post_data);

            esp_err_t err = esp_http_client_perform(client);
            
            if(err) 
            {
                ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
                return err;
            }
            
            int code = esp_http_client_get_status_code(client);
            if(code >= 200 || code <= 204)
                ESP_LOGD(TAG, "HTTP POST Status = %d", code);
            else
                ESP_LOGE(TAG, "HTTP POST Status = %d", code);
            return code;
        }

    };


    class Point
    {
        std::string measurement;
        std::vector<std::string> tagSet;
        std::vector<std::string> fieldSet;
        uint64_t timestamp;

        std::string GetQuery()
        {
            std::string query;
            query += measurement;

            // Add tags if available
            if (!tagSet.empty())
            {
                query += ",";
                for (const auto& tag : tagSet)
                {
                    query += tag + ",";
                }
                // Remove the trailing comma
                query.pop_back();
            }

            // Add fields if available
            if (!fieldSet.empty())
            {
                query += " ";
                for (const auto& field : fieldSet)
                {
                    query += field + ",";
                }
                // Remove the trailing comma
                query.pop_back();
            }

            // Add timestamp
            query += " " + std::to_string(timestamp);

            return query;
        }

    public:
        Point(const std::string measurement)
        {
            this->measurement = measurement;
        }

        Point& AddTag(const std::string name, const std::string value)
        {
            tagSet.push_back(name + "=" + value);
            return *this;
        }

        Point& AddField(const std::string name, const float value)
        {
            fieldSet.push_back(name + "=" + std::to_string(value));
            return *this;
        }

        Point& AddField(const std::string name, const int64_t value)
        {
            fieldSet.push_back(name + "=" + std::to_string(value));
            return *this;
        }

        Point& AddField(const std::string name, const uint64_t value)
        {
            fieldSet.push_back(name + "=" + std::to_string(value));
            return *this;
        }

        Point& AddField(const std::string name, const std::string value)
        {
            fieldSet.push_back(name + "=" + value);
            return *this;
        }

        Point& AddField(const std::string name, bool value)
        {
            fieldSet.push_back(name + "=" + (value?"t":"f"));
            return *this;
        }

        Point& SetTimestamp(const uint64_t unix)
        {
            timestamp = unix;
            return *this;
        }

        Point& SetTimestampNow()
        {
            time_t t = time(nullptr);
            timestamp = t * 1000000000;
            return *this;
        }

        void Post(Client& client)
        {
            client.SendLine(GetQuery());
        }
    };
}
