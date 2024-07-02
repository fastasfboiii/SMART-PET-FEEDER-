
#include <ESP32Servo.h>  
//#include <DHT.h>         
#include <Stepper.h>  
#include <WiFi.h>
#include <cstdarg>
#include <cstdlib>
#include <string.h>
#include <time.h>
// For hmac SHA256 encryption
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
// Libraries for MQTT client and WiFi connection
#include <WiFi.h>
#include <mqtt_client.h>
// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>
// Additional sample headers
#include "AzureIoT.h"
#include "Azure_IoT_PnP_Template.h"
#include "iot_configs.h"



/* --- Sample-specific Settings --- */
#define SERIAL_LOGGER_BAUD_RATE 19200
#define MQTT_DO_NOT_RETAIN_MSG 0
/* --- Time and NTP Settings --- */
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF 1
#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)
#define UNIX_TIME_NOV_13_2017 1510592825
#define UNIX_EPOCH_START_YEAR 1900
/* --- Function Returns --- */
#define RESULT_OK 0
#define RESULT_ERROR __LINE__
//#define trigPin 14
//#define echoPin 27
#define DHTPIN 4 
#define waterLevelPin 35
#define irSensorPin 17
//#define servoPin 32 //changed from 16
#define  motorIn1 26
#define  motorIn2 15
#define  motorEnA 33 // changed from 25 to 33 
#define stepperPin1 18
#define stepperPin2 19          
#define stepperPin3 21          
#define stepperPin4 22          
#define stepsPerRevolution 2048



          
/* --- Handling iot_config.h Settings --- */
static const char* wifi_ssid = "Slhboi_phone";
static const char* wifi_password = "Morinkashu222";
/* --- Function Declarations --- */
static void sync_device_clock_with_ntp_server();
static void connect_to_wifi();
const unsigned long interval1 = 4 * 60 * 60 * 1000UL; // 4 hours in milliseconds
const unsigned long interval2 = 1 * 60 * 60 * 1000UL; // 1 hour in milliseconds
const unsigned long interval1_test = 180000; // 4 hours in milliseconds
const unsigned long interval2_test = 180000; // 1 hour in milliseconds
unsigned long start_time = millis();

//ESP32_Servo_h Servo myservo;
Stepper mystepper(stepsPerRevolution, stepperPin1, stepperPin3, stepperPin2, stepperPin4);
//DHT dht(DHTPIN, DHT11);




//long duration;
//int distance;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis = 0;
unsigned long interval = 10000;


//////////////////////////////////////////////////////////////////////////Azure///////////////////////////////////////////////////////////////////

  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  static void esp_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
  #else // ESP_ARDUINO_VERSION_MAJOR
  static esp_err_t esp_mqtt_event_handler(esp_mqtt_event_handle_t event);
  #endif // ESP_ARDUINO_VERSION_MAJOR
  // This is a logging function used by Azure IoT client.
  static void logging_function(log_level_t log_level, char const* const format, ...);
  /* --- Sample variables --- */
  static azure_iot_config_t azure_iot_config;
  static azure_iot_t azure_iot;
  static esp_mqtt_client_handle_t mqtt_client;
  static char mqtt_broker_uri[128];
  #define AZ_IOT_DATA_BUFFER_SIZE 1500
  static uint8_t az_iot_data_buffer[AZ_IOT_DATA_BUFFER_SIZE];
  #define MQTT_PROTOCOL_PREFIX "mqtts://"
  static uint32_t properties_request_id = 0;
  static bool send_device_info = true;
  static bool azure_initial_connect = false; //Turns true when ESP32 successfully connects to Azure IoT Central for the first time
  static int mqtt_client_init_function(
      mqtt_client_config_t* mqtt_client_config,
      mqtt_client_handle_t* mqtt_client_handle)
  {
    int result;
    esp_mqtt_client_config_t mqtt_config;
    memset(&mqtt_config, 0, sizeof(mqtt_config));

    az_span mqtt_broker_uri_span = AZ_SPAN_FROM_BUFFER(mqtt_broker_uri);
    mqtt_broker_uri_span = az_span_copy(mqtt_broker_uri_span, AZ_SPAN_FROM_STR(MQTT_PROTOCOL_PREFIX));
    mqtt_broker_uri_span = az_span_copy(mqtt_broker_uri_span, mqtt_client_config->address);
    az_span_copy_u8(mqtt_broker_uri_span, null_terminator);

  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
      mqtt_config.broker.address.uri = mqtt_broker_uri;
      mqtt_config.broker.address.port = mqtt_client_config->port;
      mqtt_config.credentials.client_id = (const char*)az_span_ptr(mqtt_client_config->client_id);
      mqtt_config.credentials.username = (const char*)az_span_ptr(mqtt_client_config->username);

    #ifdef IOT_CONFIG_USE_X509_CERT
      LogInfo("MQTT client using X509 Certificate authentication");
      mqtt_config.credentials.authentication.certificate = IOT_CONFIG_DEVICE_CERT;
      mqtt_config.credentials.authentication.certificate_len = (size_t)sizeof(IOT_CONFIG_DEVICE_CERT);
      mqtt_config.credentials.authentication.key = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
      mqtt_config.credentials.authentication.key_len = (size_t)sizeof(IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY);
    #else // Using SAS key
      mqtt_config.credentials.authentication.password = (const char*)az_span_ptr(mqtt_client_config->password);
    #endif

      mqtt_config.session.keepalive = 30;
      mqtt_config.session.disable_clean_session = 0;
      mqtt_config.network.disable_auto_reconnect = false;
      mqtt_config.broker.verification.certificate = (const char*)ca_pem;
      mqtt_config.broker.verification.certificate_len = (size_t)ca_pem_len;
  #else // ESP_ARDUINO_VERSION_MAJOR
      mqtt_config.uri = mqtt_broker_uri;
      mqtt_config.port = mqtt_client_config->port;
      mqtt_config.client_id = (const char*)az_span_ptr(mqtt_client_config->client_id);
      mqtt_config.username = (const char*)az_span_ptr(mqtt_client_config->username);

    #ifdef IOT_CONFIG_USE_X509_CERT
      LogInfo("MQTT client using X509 Certificate authentication");
      mqtt_config.client_cert_pem = IOT_CONFIG_DEVICE_CERT;
      mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
    #else // Using SAS key
      mqtt_config.password = (const char*)az_span_ptr(mqtt_client_config->password);
    #endif

      mqtt_config.keepalive = 30;
      mqtt_config.disable_clean_session = 0;
      mqtt_config.disable_auto_reconnect = false;
      mqtt_config.event_handle = esp_mqtt_event_handler;
      mqtt_config.user_context = NULL;
      mqtt_config.cert_pem = (const char*)ca_pem;
  #endif // ESP_ARDUINO_VERSION_MAJOR

    LogInfo("MQTT client target uri set to '%s'", mqtt_broker_uri);

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL)
    {
      LogError("esp_mqtt_client_init failed.");
      result = 1;
    }
    else
    {
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
      esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, esp_mqtt_event_handler, NULL);
  #endif // ESP_ARDUINO_VERSION_MAJOR

      esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

      if (start_result != ESP_OK)
      {
        LogError("esp_mqtt_client_start failed (error code: 0x%08x).", start_result);
        result = 1;
      }
      else
      {
        *mqtt_client_handle = mqtt_client;
        result = 0;
      }
    }

    return result;
  }
  /*
  * See the documentation of `mqtt_client_deinit_function_t` in AzureIoT.h for details.
  */
  static int mqtt_client_deinit_function(mqtt_client_handle_t mqtt_client_handle)
  {
    int result = 0;
    esp_mqtt_client_handle_t esp_mqtt_client_handle = (esp_mqtt_client_handle_t)mqtt_client_handle;

    LogInfo("MQTT client being disconnected.");

    if (esp_mqtt_client_stop(esp_mqtt_client_handle) != ESP_OK)
    {
      LogError("Failed stopping MQTT client.");
    }

    if (esp_mqtt_client_destroy(esp_mqtt_client_handle) != ESP_OK)
    {
      LogError("Failed destroying MQTT client.");
    }

    if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)
    {
      LogError("Failed updating azure iot client of MQTT disconnection.");
    }

    return 0;
  }
  /*
  * See the documentation of `mqtt_client_subscribe_function_t` in AzureIoT.h for details.
  */
  static int mqtt_client_subscribe_function(
      mqtt_client_handle_t mqtt_client_handle,
      az_span topic,
      mqtt_qos_t qos)
  {
    LogInfo("MQTT client subscribing to '%.*s'", az_span_size(topic), az_span_ptr(topic));

    // As per documentation, `topic` always ends with a null-terminator.
    // esp_mqtt_client_subscribe returns the packet id or negative on error already, so no conversion
    // is needed.
    int packet_id = esp_mqtt_client_subscribe(
        (esp_mqtt_client_handle_t)mqtt_client_handle, (const char*)az_span_ptr(topic), (int)qos);

    return packet_id;
  }
  /*
  * See the documentation of `mqtt_client_publish_function_t` in AzureIoT.h for details.
  */
  static int mqtt_client_publish_function(
      mqtt_client_handle_t mqtt_client_handle,
      mqtt_message_t* mqtt_message)
  {
    LogInfo("MQTT client publishing to '%s'", az_span_ptr(mqtt_message->topic));

    int mqtt_result = esp_mqtt_client_publish(
        (esp_mqtt_client_handle_t)mqtt_client_handle,
        (const char*)az_span_ptr(mqtt_message->topic), // topic is always null-terminated.
        (const char*)az_span_ptr(mqtt_message->payload),
        az_span_size(mqtt_message->payload),
        (int)mqtt_message->qos,
        MQTT_DO_NOT_RETAIN_MSG);

    if (mqtt_result == -1)
    {
      return RESULT_ERROR;
    }
    else
    {
      return RESULT_OK;
    }
  }
  /* --- Other Interface functions required by Azure IoT --- */
  /*
  * See the documentation of `hmac_sha256_encryption_function_t` in AzureIoT.h for details.
  */
  static int mbedtls_hmac_sha256(
      const uint8_t* key,
      size_t key_length,
      const uint8_t* payload,
      size_t payload_length,
      uint8_t* signed_payload,
      size_t signed_payload_size)
  {
    (void)signed_payload_size;
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key, key_length);
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)payload, payload_length);
    mbedtls_md_hmac_finish(&ctx, (byte*)signed_payload);
    mbedtls_md_free(&ctx);

    return 0;
  }

  /*
  * See the documentation of `base64_decode_function_t` in AzureIoT.h for details.
  */
  static int base64_decode(
      uint8_t* data,
      size_t data_length,
      uint8_t* decoded,
      size_t decoded_size,
      size_t* decoded_length)
  {
    return mbedtls_base64_decode(decoded, decoded_size, decoded_length, data, data_length);
  }

  /*
  * See the documentation of `base64_encode_function_t` in AzureIoT.h for details.
  */
  static int base64_encode(
      uint8_t* data,
      size_t data_length,
      uint8_t* encoded,
      size_t encoded_size,
      size_t* encoded_length)
  {
    return mbedtls_base64_encode(encoded, encoded_size, encoded_length, data, data_length);
  }

  /*
  * See the documentation of `properties_update_completed_t` in AzureIoT.h for details.
  */
  static void on_properties_update_completed(uint32_t request_id, az_iot_status status_code)
  {
    LogInfo("Properties update request completed (id=%d, status=%d)", request_id, status_code);
  }
  /*
  * See the documentation of `properties_received_t` in AzureIoT.h for details.
  */
  void on_properties_received(az_span properties)
  {
    LogInfo("Properties update received: %.*s", az_span_size(properties), az_span_ptr(properties));

    // It is recommended not to perform work within callbacks.
    // The properties are being handled here to simplify the sample.
    if (azure_pnp_handle_properties_update(&azure_iot, properties, properties_request_id++) != 0)
    {
      LogError("Failed handling properties update.");
    }
  }
  /*
  * See the documentation of `command_request_received_t` in AzureIoT.h for details.
  */
  static void on_command_request_received(command_request_t command)
  {
    az_span component_name
        = az_span_size(command.component_name) == 0 ? AZ_SPAN_FROM_STR("") : command.component_name;

    LogInfo(
        "Command request received (id=%.*s, component=%.*s, name=%.*s)",
        az_span_size(command.request_id),
        az_span_ptr(command.request_id),
        az_span_size(component_name),
        az_span_ptr(component_name),
        az_span_size(command.command_name),
        az_span_ptr(command.command_name));

    // Here the request is being processed within the callback that delivers the command request.
    // However, for production application the recommendation is to save `command` and process it
    // outside this callback, usually inside the main thread/task/loop.
    (void)azure_pnp_handle_command_request(&azure_iot, command);
  }
  static void configure_azure_iot() {
    /*
    * The configuration structure used by Azure IoT must remain unchanged (including data buffer)
    * throughout the lifetime of the sample. This variable must also not lose context so other
    * components do not overwrite any information within this structure.
    */
    azure_iot_config.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);
    azure_iot_config.model_id = azure_pnp_get_model_id();
    azure_iot_config.use_device_provisioning = true; // Required for Azure IoT Central.
    azure_iot_config.iot_hub_fqdn = AZ_SPAN_EMPTY;
    azure_iot_config.device_id = AZ_SPAN_EMPTY;

  #ifdef IOT_CONFIG_USE_X509_CERT
    azure_iot_config.device_certificate = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_CERT);
    azure_iot_config.device_certificate_private_key
        = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY);
    azure_iot_config.device_key = AZ_SPAN_EMPTY;
  #else
    azure_iot_config.device_certificate = AZ_SPAN_EMPTY;
    azure_iot_config.device_certificate_private_key = AZ_SPAN_EMPTY;
    azure_iot_config.device_key = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY);
  #endif // IOT_CONFIG_USE_X509_CERT

    azure_iot_config.dps_id_scope = AZ_SPAN_FROM_STR(DPS_ID_SCOPE);
    azure_iot_config.dps_registration_id
        = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID); // Use Device ID for Azure IoT Central.
    azure_iot_config.data_buffer = AZ_SPAN_FROM_BUFFER(az_iot_data_buffer);
    azure_iot_config.sas_token_lifetime_in_minutes = MQTT_PASSWORD_LIFETIME_IN_MINUTES;
    azure_iot_config.mqtt_client_interface.mqtt_client_init = mqtt_client_init_function;
    azure_iot_config.mqtt_client_interface.mqtt_client_deinit = mqtt_client_deinit_function;
    azure_iot_config.mqtt_client_interface.mqtt_client_subscribe = mqtt_client_subscribe_function;
    azure_iot_config.mqtt_client_interface.mqtt_client_publish = mqtt_client_publish_function;
    azure_iot_config.data_manipulation_functions.hmac_sha256_encrypt = mbedtls_hmac_sha256;
    azure_iot_config.data_manipulation_functions.base64_decode = base64_decode;
    azure_iot_config.data_manipulation_functions.base64_encode = base64_encode;
    azure_iot_config.on_properties_update_completed = on_properties_update_completed;
    azure_iot_config.on_properties_received = on_properties_received;
    azure_iot_config.on_command_request_received = on_command_request_received;

    azure_iot_init(&azure_iot, &azure_iot_config);
  }

//////////////////////////////////////////////////////////////////////////Azure end///////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////test Azure//////////////////////////////////////////////////////////////

    static void sync_device_clock_with_ntp_server()
  {
    LogInfo("Setting time using SNTP");

    configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
    time_t now = time(NULL);
    while (now < UNIX_TIME_NOV_13_2017)
    {
      delay(500);
      Serial.print(".");
      now = time(NULL);
    }
    Serial.println("");
    LogInfo("Time initialized!");
  }

  static void connect_to_wifi()
  {
    LogInfo("Connecting to WIFI wifi_ssid %s", wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");

    LogInfo("WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
  }

  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  static void esp_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
  {
    (void)handler_args;
    (void)base;
    (void)event_id;

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  #else // ESP_ARDUINO_VERSION_MAJOR
  static esp_err_t esp_mqtt_event_handler(esp_mqtt_event_handle_t event)
  {
  #endif // ESP_ARDUINO_VERSION_MAJOR
    switch (event->event_id)
    {
      int i, r;

      case MQTT_EVENT_ERROR:
        LogError("MQTT client in ERROR state.");
        LogError(
            "esp_tls_stack_err=%d; "
            "esp_tls_cert_verify_flags=%d;esp_transport_sock_errno=%d;error_type=%d;connect_return_"
            "code=%d",
            event->error_handle->esp_tls_stack_err,
            event->error_handle->esp_tls_cert_verify_flags,
            event->error_handle->esp_transport_sock_errno,
            event->error_handle->error_type,
            event->error_handle->connect_return_code);

        switch (event->error_handle->connect_return_code)
        {
          case MQTT_CONNECTION_ACCEPTED:
            LogError("connect_return_code=MQTT_CONNECTION_ACCEPTED");
            break;
          case MQTT_CONNECTION_REFUSE_PROTOCOL:
            LogError("connect_return_code=MQTT_CONNECTION_REFUSE_PROTOCOL");
            break;
          case MQTT_CONNECTION_REFUSE_ID_REJECTED:
            LogError("connect_return_code=MQTT_CONNECTION_REFUSE_ID_REJECTED");
            break;
          case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
            LogError("connect_return_code=MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE");
            break;
          case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
            LogError("connect_return_code=MQTT_CONNECTION_REFUSE_BAD_USERNAME");
            break;
          case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
            LogError("connect_return_code=MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED");
            break;
          default:
            LogError("connect_return_code=unknown (%d)", event->error_handle->connect_return_code);
            break;
        };

        break;
      case MQTT_EVENT_CONNECTED:
        LogInfo("MQTT client connected (session_present=%d).", event->session_present);

        if (azure_iot_mqtt_client_connected(&azure_iot) != 0)
        {
          LogError("azure_iot_mqtt_client_connected failed.");
        }

        break;
      case MQTT_EVENT_DISCONNECTED:
        LogInfo("MQTT client disconnected.");

        if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)
        {
          LogError("azure_iot_mqtt_client_disconnected failed.");
        }

        break;
      case MQTT_EVENT_SUBSCRIBED:
        LogInfo("MQTT topic subscribed (message id=%d).", event->msg_id);

        if (azure_iot_mqtt_client_subscribe_completed(&azure_iot, event->msg_id) != 0)
        {
          LogError("azure_iot_mqtt_client_subscribe_completed failed.");
        }

        break;
      case MQTT_EVENT_UNSUBSCRIBED:
        LogInfo("MQTT topic unsubscribed.");
        break;
      case MQTT_EVENT_PUBLISHED:
        LogInfo("MQTT event MQTT_EVENT_PUBLISHED");

        if (azure_iot_mqtt_client_publish_completed(&azure_iot, event->msg_id) != 0)
        {
          LogError("azure_iot_mqtt_client_publish_completed failed (message id=%d).", event->msg_id);
        }

        break;
      case MQTT_EVENT_DATA:
        LogInfo("MQTT message received.");

        mqtt_message_t mqtt_message;
        mqtt_message.topic = az_span_create((uint8_t*)event->topic, event->topic_len);
        mqtt_message.payload = az_span_create((uint8_t*)event->data, event->data_len);
        mqtt_message.qos
            = mqtt_qos_at_most_once; // QoS is unused by azure_iot_mqtt_client_message_received.

        if (azure_iot_mqtt_client_message_received(&azure_iot, &mqtt_message) != 0)
        {
          LogError(
              "azure_iot_mqtt_client_message_received failed (topic=%.*s).",
              event->topic_len,
              event->topic);
        }

        break;
      case MQTT_EVENT_BEFORE_CONNECT:
        LogInfo("MQTT client connecting.");
        break;
      default:
        LogError("MQTT event UNKNOWN.");
        break;
    }

  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  #else // ESP_ARDUINO_VERSION_MAJOR
    return ESP_OK;
  #endif // ESP_ARDUINO_VERSION_MAJOR
  }

  static void logging_function(log_level_t log_level, char const* const format, ...)
  {
    struct tm* ptm;
    time_t now = time(NULL);

    ptm = gmtime(&now);

    Serial.print(ptm->tm_year + UNIX_EPOCH_START_YEAR);
    Serial.print("/");
    Serial.print(ptm->tm_mon + 1);
    Serial.print("/");
    Serial.print(ptm->tm_mday);
    Serial.print(" ");

    if (ptm->tm_hour < 10)
    {
      Serial.print(0);
    }

    Serial.print(ptm->tm_hour);
    Serial.print(":");

    if (ptm->tm_min < 10)
    {
      Serial.print(0);
    }

    Serial.print(ptm->tm_min);
    Serial.print(":");

    if (ptm->tm_sec < 10)
    {
      Serial.print(0);
    }

    Serial.print(ptm->tm_sec);

    Serial.print(log_level == log_level_info ? " [INFO] " : " [ERROR] ");

    char message[256];
    va_list ap;
    va_start(ap, format);
    int message_length = vsnprintf(message, 256, format, ap);
    va_end(ap);

    if (message_length < 0)
    {
      Serial.println("Failed encoding log message (!)");
    }
    else
    {
      Serial.println(message);
    }
  }

/////////////////////////////////////////////////////////////////////////test Azure end//////////////////////////////////////////////////////////////

void start (){
  if (WiFi.status() != WL_CONNECTED)
  {
    azure_iot_stop(&azure_iot);
    
    connect_to_wifi();
    
    if (!azure_initial_connect)
    {
      configure_azure_iot();
    }
    
    azure_iot_start(&azure_iot);
  }
  else
  {
    switch (azure_iot_get_status(&azure_iot))
    {
      case azure_iot_connected:
        azure_initial_connect = true;

        if (send_device_info)
        {
          (void)azure_pnp_send_device_info(&azure_iot, properties_request_id++);
          send_device_info = false; // Only need to send once.
        }
        else if (azure_pnp_send_telemetry(&azure_iot) != 0)
        {
          LogError("Failed sending telemetry.");
        }
        break;
        
      case azure_iot_error:
        LogError("Azure IoT client is in error state.");
        azure_iot_stop(&azure_iot);
        break;
        
      case azure_iot_disconnected:
        WiFi.disconnect();
        break;
        
      default:
        break;
    }

    azure_iot_do_work(&azure_iot);
  }
}
// Function to move the motor forward
void moveMotorForward() {
  digitalWrite(motorIn1, HIGH);
  digitalWrite(motorIn2, LOW);
  analogWrite(motorEnA, 255); // Set motor speed (0-255)
}

// Function to stop the motor
void stopMotor() {
  digitalWrite(motorIn1, LOW);
  digitalWrite(motorIn2, LOW);
}

// void check_humid() {
//   float measuredH = dht.readHumidity();

//   if (isnan(measuredH)) {
//     Serial.println("Failed to read from DHT sensor!");
//   } else {
//     Serial.print("Humidity: ");
//     Serial.print(measuredH);
//     Serial.print(" %\t");
//   }
// }
// void check_temp() {
//   float measuredT = dht.readTemperature();

//   if (isnan(measuredT)) {
//     Serial.println("Failed to read from DHT sensor!");
//   } else {
//     Serial.print("Temperature: ");
//     Serial.print(measuredT);
//     Serial.println(" *C");
//   }
// }

void dispense_food() {
    mystepper.step(stepsPerRevolution/2); // Move a quarter turn
    Serial.println("Food valve opened");
    moveMotorForward();  // Set motor speed (0-255)
    delay(5000);
    stopMotor();
    mystepper.step(-stepsPerRevolution/2);
    Serial.println("Food valve closed");
}

// void check_water() {
//   int waterLevelValue = analogRead(waterLevelPin);
//   float waterLevel = map(waterLevelValue, 0, 4095, 0, 100); // Map analog value to percentage

//   Serial.print("Water Level: ");
//   Serial.print(waterLevel);
//   Serial.println(" %");

//   if (waterLevel <= 10){
//     Serial.println("Low water level");
//   }
// }

// void check_food() {
//   delay(50);
//   int irValue = digitalRead(irSensorPin);
//   delay(50);

//   if (irValue == LOW) { // Assuming LOW means it detects food
//     Serial.println("There is food in the bowl");
//     delay(50);
//   } 
//   else {
//     Serial.println("No food in the bowl");
//     delay(50);

//   }
// }

void control() {
  unsigned long currentMillis = millis();

  // Check if it's time to light the LED for the 4-hour interval
  if (currentMillis - previousMillis1 >= interval1_test) {
    previousMillis1 = currentMillis;
    
    // check_food();
    Serial.println("will dispence");
    dispense_food();
  }

  // Check if it's time to light the LED for the 1-hour interval
  if (currentMillis - previousMillis2 >= interval2_test) {
    previousMillis2 = currentMillis;
    // check_water();
    
  }
}


// void moveServoRandomly(int duration) {  
//   unsigned long startTime = millis();
//   for (int i = 0; i<10; i++){
//     int randomDirection = random(0, 180); // Random direction between 0 and 180 degrees

//     // Move the servo to the random direction
//     myservo.write(randomDirection);
//     delay(1000);
//     myservo.write(0);

//     // Print the random values to the serial monitor
//     Serial.print(", Direction: ");
//     Serial.println(randomDirection);
//     delay(100);
//   }
  //  while (millis() - startTime <= 2*duration) {
  //   // Generate a random speed and direction
  //   int randomDirection = random(0, 180); // Random direction between 0 and 180 degrees

  //   // Move the servo to the random direction
  //   myservo.write(randomDirection);
  //   delay(1000);
  //   myservo.write(0);

  //   // Print the random values to the serial monitor
  //   Serial.print("Speed: ");
  //   Serial.print(randomSpeed);
  //   Serial.print(", Direction: ");
  //   Serial.println(randomDirection);
  //   delay(100);

  //   // Wait for a random time between movements
  //   //int randomDelay = random(100, 500); // Random delay between 100 and 500 milliseconds
  //   //delay(randomDelay);
  // }
// }

// void check_ultra() {
//   digitalWrite(trigPin, LOW);
//   delayMicroseconds(2);
//   digitalWrite(trigPin, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(trigPin, LOW);

//   duration = pulseIn(echoPin, HIGH);
//   distance = duration * 0.034 / 2;

//   Serial.print("Distance: ");
//   Serial.print(distance);
//   Serial.println(" cm");

//   if (distance <= 20) {
//     moveServoRandomly(5000);

//   }
// }


void setup() {
  Serial.begin(19200);
  pinMode(irSensorPin, INPUT);
  pinMode(motorIn1, OUTPUT);
  pinMode(motorIn2, OUTPUT);
  pinMode(motorEnA, OUTPUT);
  pinMode(stepperPin1, OUTPUT);
  pinMode(stepperPin2, OUTPUT);
  pinMode(stepperPin3, OUTPUT);
  pinMode(stepperPin4, OUTPUT);

  //myservo.attach(servoPin);
  mystepper.setSpeed(10);
 // dht.begin();
  set_logging_function(logging_function);
  connect_to_wifi();
  sync_device_clock_with_ntp_server();
  azure_pnp_init();
  configure_azure_iot();
  azure_iot_start(&azure_iot);
  LogInfo("Azure IoT client initialized (state=%d)", azure_iot.state);
  Serial.println("Ultrasonic sensor, servo, DHT11 sensor, water level sensor, IR sensor, and motor test begins");

}


void loop() {
    start();
  // check_ultra();
  // delay(50);
  check_humid();
  delay(50);
  check_temperature();
  delay(50);
  check_food();
  delay(50);
  check_water();
  delay(50);
  control();
  delay(50);

  delay(100); 
  if (millis() - start_time > 30000){
    unsigned long startTime = millis();
    while (millis() - startTime < interval) {
      Serial.println("sending");
      const size_t bufferSize = 1024;
      uint8_t payloadBuffer[bufferSize];
      size_t payloadLength;

      if (generate_telemetry_payload(payloadBuffer, bufferSize, &payloadLength) == RESULT_OK) {
        az_span telemetry_payload = az_span_create(payloadBuffer, payloadLength);
        if (azure_iot_send_telemetry(&azure_iot, telemetry_payload) == RESULT_OK) {
          Serial.println("Telemetry sent successfully");
        } else {
          Serial.println("Failed to send telemetry");
        }
      } else {
        Serial.println("Failed to generate telemetry payload");
      }

      delay(5000); // Adjust the delay as needed
    }
  }
  
}


