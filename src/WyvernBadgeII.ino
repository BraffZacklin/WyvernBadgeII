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
	lcd.begin(16,2);
	lcd.setCursor(0,0);
	lcd.leftToRight();

	initWIFI();
	displayIntro();
	initNames();
	displayFronter();
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
	sysName = getValueByKeyWithUrl("name", APISystemURL);
}

void displayFronter()
{
	char* newName = getValueByKeyWithUrl("name", APIFronterURL);

	if (strcmp(newName, fronterName) != 0)
	{
		strcpy(fronterName, newName);
		int nameLen = strlen(fronterName);

		lcd.clear();
		lcd.setCursor(0,0);
		lcd.write(sysName);

		lcd.setCursor(15-nameLen, 1);
		lcd.write(fronterName);
	}
}

void loop()
{
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

char* getValueByKeyWithUrl(char* key, char* url)
{
	jsmn_parser parser;
	int token_count;
	String response_Str;
	char* value;
	WiFiClientSecure client;

	client.setInsecure();
	client.connect(APIHostname, HTTPSPort);
	
	if (client.verify(APICertFingerprint, APIHostname))
	{
		client.print(String("GET ") + url + " HTTP/1.1\r\n" +
							"Host: " + APIHostname + "\r\n" +               
							"Connection: close\r\n" +
							"Cookies: Authorization=" + Token + "\r\n\r\n");


		while (client.connected())
		{
			if (client.readStringUntil("\n") == "\r")
			{
				break;
			}
		}

		while (client.available())
		{
			response_Str = client.readString();
		}

	}

	jsmn_init(&parser);
	token_count = jsmn_parse(&parser, response_Str.c_str(), strlen(response_Str.c_str()), NULL, 1);

	jsmn_init(&parser);
	jsmntok_t token_array[token_count];
	int parse_response = jsmn_parse(&parser, response_Str.c_str(), strlen(response_Str.c_str()), token_array, token_count);

	if (parse_response > 0)
	{
		for (int a = 0; a < token_count; a++)
		{
			value = getValueByKey(response_Str.c_str(), token_array, a, key);
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
