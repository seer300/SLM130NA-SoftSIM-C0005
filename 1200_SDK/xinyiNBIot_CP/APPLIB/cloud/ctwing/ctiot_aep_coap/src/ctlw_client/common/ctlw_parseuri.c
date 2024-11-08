/************************************************************************

            (c) Copyright 2018 by 中国电信上海研究院. All rights reserved.

**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ctlw_liblwm2m.h"
#include "ctlw_parseuri.h"

#define MAX_DOMAIN_LEN 255

bool ctiot_parse_url(char *url, CTIOT_URI *uri)
{
    //printf("url:%s\n",url);
    if (url == NULL || strlen(url) < 15)
    {
        return false;
    }
    if (uri == NULL)
    {
        printf("malloc failed\n");
        return false;
    }
    memset(uri->protocol, 0x00, sizeof(uri->protocol));
    uri->port = 0x00;
    
    int protocollen = 8;
    if (strncasecmp(url, "https", 5) == 0)
    {
        protocollen = 8;
        memcpy(uri->protocol, url, 5);
        uri->port = 443;
    }
    else if (strncasecmp(url, "coaps", 5) == 0)
    {
        protocollen = 8;
        memcpy(uri->protocol, url, 5);
        uri->port = 5684;
    }
    else if (strncasecmp(url, "http", 4) == 0)
    {
        protocollen = 7;
        memcpy(uri->protocol, url, 4);
        uri->port = 80;
    }
    else if (strncasecmp(url, "coap", 4) == 0)
    {
        protocollen = 7;
        memcpy(uri->protocol, url, 4);
        uri->port = 5683;
    }
    else
    {
        printf("no coap/http protocol header found\n");
        return false;
    }
    //printf("protocol:%s\n",uri->protocol);


	if(uri->address != NULL)
	{
		xy_free(uri->address);
	}
	uri->address = xy_malloc2(strlen(url));

    if(uri->address == NULL)
        return false;

	memset(uri->address, 0x00, strlen(url));
   

    int i = protocollen;
	int v6 = 0;
	if(url[i] == '[')
	{
		v6 = 1;
		i ++;
	}
    for (; i < strlen(url); i++)
    {
		if(v6 == 0)
		{
	        if (url[i] != 47 && url[i] != 92 && url[i] != 58) //':'=58
	        {
	            uri->address[i - protocollen] = url[i];
	        }
	        else
	        {
	            break;
	        }
		}
		else
		{
			if(url[i] == ']')
			{
				i ++;
				break;
			}
			if ((url[i] >= '0' && url[i] <= '9')  ||(url[i] >= 'a' && url[i] <= 'f')  ||(url[i] >= 'A' && url[i] <= 'F')  || url[i] == ':') //':'=58
	        {
	            uri->address[i - protocollen - 1] = url[i];
	        }
			else
			{
				return false;
			}
		}

		if (i > MAX_DOMAIN_LEN + protocollen)
        {
            printf("url error\n");
            return false;
        }
    }


    if (url[i] == 58)
    {
        i++;
        char tmpport[6] = {0};
        int j = 0;
        for (; i < strlen(url); i++)
        {
            if (url[i] != 47 && url[i] != 92)
            {
                if (url[i] >= 48 && url[i] <= 57) //'0'and '9'
                {
                    tmpport[j++] = url[i];
                }
            }
            else
            {
                break;
            }
            if (j >= 5)
            {
                break;
            }
        }
		if(strlen(tmpport) != 0)
		{
			uri->port = atoi(tmpport);
		}
    }

	if(uri->uri != NULL)
	{
		xy_free(uri->uri);
	}

	uri->uri = xy_malloc2(strlen(url)-i+1);

    if(uri->uri == NULL)
        return false;

    
	memset(uri->uri, 0x00, strlen(url)-i+1);

    int start = 0;
    int pos = 0;
    for (; i < strlen(url); i++)
    {
        if (url[i] == 47 || url[i] == 92) // '\'=92 '/'=47
        {
            start = 1;
        }
        if (start == 1)
        {
            uri->uri[pos++] = url[i];
        }
    }

    return true;
}
