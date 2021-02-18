#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <string.h>
#include <jsmn.h>
#include <stdlib.h>
#include "system.h"

#define MaxNameLen 16
#define EllipsisDelayTime 500
#define OpenScreenWaitTime 3000 - 3 * EllipsisDelayTime
#define HTTPSPort 443

LiquidCrystal lcd(D5, D6, D4, D3, D2, D1);

char* sysName;
char fronterName[MaxNameLen + 1];
unsigned long myTime;

void displayFronter();
void displayIntro();
void initNames();
void initWIFI();

void setup()
{
	//debug
	Serial.begin(115200);
	while (!Serial)
		{;}

	lcd.begin(16,2);
	lcd.setCursor(0,0);
	lcd.leftToRight();

	initWIFI();
	displayIntro();
	initNames();
	displayFronter();

	//debug
	Serial.println("Finished Setup");
}

void displayIntro()
{
	lcd.write("Here there be");
	lcd.setCursor(0,1);
	lcd.write("Wyverns");
	for (int i = 0; i < 3; i++)
	{
		delay(EllipsisDelayTime);
		lcd.write(".");
	}
	delay(OpenScreenWaitTime);
}

void initNames()
{
	sysName = curlValueByKeyWithUrl("name", APISystemURL);
}

void displayFronter()
{
	char* newName = curlValueByKeyWithUrl("name", APIFronterURL);

	if (strcmp(newName, fronterName) != 0)
	{
		//debug
		Serial.println("1");
		strcpy(fronterName, newName);
		//debug
		Serial.println("3");
		lcd.clear();
		//debug
		Serial.println("4");

		lcd.setCursor(0,0);
		//debug
		Serial.println("5");
		lcd.write(sysName);
		//debug
		Serial.println("6");

		int nameLen = strlen(fronterName);
		//debug
		Serial.println("7");
		lcd.setCursor(15-nameLen, 1);
		//debug
		Serial.println("8");
		lcd.write(fronterName);
		//debug
		Serial.println("9");
	}
}

void loop()
{
	//debug
	Serial.println("Beginning Loop");
	
	delay(60*1000);
	displayFronter();
}

void initWIFI()
{
	WiFi.begin(SSID, Password);
	lcd.write("Connecting");
	while (WiFi.status() != WL_CONNECTED) 
	{
		for (int a = 0; a < 16 - strlen("Connecting"); a++)
		{
			delay(EllipsisDelayTime);
			lcd.write(".");
		}
		lcd.setCursor(0,1);
		for (int a = 0; a < 16; a++)
		{
			delay(EllipsisDelayTime);
			lcd.write(".");
		}
	}
	lcd.setCursor(0,0);
	lcd.clear();

}

char* getValueByKey(const char *json_string, jsmntok_t token_array[], int index, const char* key);

char* curlValueByKeyWithUrl(char* key, char* url)
{
	jsmn_parser parser;
	int token_count;
	String response_Str;
	char* value;
	WiFiClientSecure client;

	client.setInsecure();
	client.connect(APIHostname, HTTPSPort);
	
	//debug
	Serial.println("Connection Established");

	if (client.verify(APICertFingerprint, APIHostname))
	{
		//debug
		Serial.print("Sending Request");
		Serial.println(String("GET ") + url + " HTTP/1.1\r\n" +
							"Host: " + APIHostname + "\r\n" +               
							"Connection: close\r\n" +
							"Cookies: Authorization=" + Token + "\r\n\r\n");
		client.print(String("GET ") + url + " HTTP/1.1\r\n" +
							"Host: " + APIHostname + "\r\n" +               
							"Connection: close\r\n" +
							"Cookies: Authorization=" + Token + "\r\n\r\n");


		Serial.println("HEADERS:");
		while (client.connected())
		{
			response_Str = client.readStringUntil('\n');
			{
				//debug
				Serial.println(response_Str);
				if (response_Str == "\r")
				{
					//debug
					Serial.println("Headers Received");
					break;
				}
			}
		}

		while (client.available())
		{
			response_Str = client.readString();
			//debug
			Serial.println(response_Str);
		}

	}
	else
	{
		Serial.println("API Cert Fingerprint Not Matching");
	}

	jsmn_init(&parser);
	token_count = jsmn_parse(&parser, response_Str.c_str(), strlen(response_Str.c_str()), NULL, 1);
	
	//debug
	Serial.println("Token count: ");
	Serial.println(token_count);

	jsmn_init(&parser);
	jsmntok_t token_array[token_count];
	int parse_response = jsmn_parse(&parser, response_Str.c_str(), strlen(response_Str.c_str()), token_array, token_count);

	Serial.print("Parse Response: ");
	Serial.println(parse_response);

	if (parse_response > 0)
	{
		for (int a = 0; a < token_count; a++)
		{
			value = getValueByKey(response_Str.c_str(), token_array, a, key);
			Serial.println(value);
			if (value != NULL)
			{
				break;
			}
		}
	}
	return value;
}

char* getValueByKey(const char *json_string, jsmntok_t token_array[], int index, const char* key)
{
	jsmntok_t* key_token = &token_array[index];
	if (key_token->type == JSMN_STRING)
	{
		if ((int) strlen(key) == key_token->end - key_token->start)
		{
			if (strncmp(json_string + key_token->start, key, key_token->end - key_token->start) == 0)
			{
				jsmntok_t* value_token = &token_array[index + 1];
				size_t value_size = (size_t) value_token->end - value_token->start;
				char* value = strndup(json_string + value_token->start, value_size);
				return value;
			}
		}
	}
	return NULL;
}
